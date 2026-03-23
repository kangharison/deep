/* rpc.c - DIS/SISCI 기반 원격 프로시저 호출(RPC) 구현
 *
 * 이 파일은 DIS 패브릭을 통해 클러스터 내 노드 간 NVMe Admin 명령을
 * 원격으로 실행하기 위한 RPC 메커니즘을 구현한다.
 *
 * 아키텍처:
 * - 서버(핸들러): NVMe 디바이스를 물리적으로 소유한 노드가 RPC 바인딩 핸들을 생성하고,
 *   데이터 인터럽트로 원격 명령을 수신하여 로컬에서 실행한 뒤 결과를 돌려보낸다.
 * - 클라이언트(바인딩): 원격 노드가 디바이스 공유 메모리에서 핸들러 정보를 읽고,
 *   데이터 인터럽트를 통해 명령을 보내고 응답을 기다린다.
 *
 * 통신 흐름:
 *   클라이언트 → (SCITriggerDataInterrupt) → 서버: RPC 명령 전송
 *   서버: 로컬 Admin Queue에서 명령 실행
 *   서버 → (SCITriggerDataInterrupt) → 클라이언트: 실행 결과 반환
 */

/* _SISCI 매크로가 정의되지 않으면 컴파일을 중단한다 */
#ifndef _SISCI
#error "Must compile with SISCI support"
#endif

/* DIS 클러스터 모드 활성화 */
#ifndef __DIS_CLUSTER__
#define __DIS_CLUSTER__
#endif

#include <nvm_types.h>      /* NVM 공통 타입 정의 */
#include <nvm_rpc.h>        /* RPC 공개 API */
#include <nvm_aq.h>         /* Admin Queue 참조 */
#include <nvm_util.h>       /* 유틸리티 매크로 */
#include <nvm_queue.h>      /* NVMe 큐 관련 */
#include <nvm_error.h>      /* NVM_ERR_PACK 에러 패킹 매크로 */
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "dis/device.h"     /* struct device, 세그먼트 함수 */
#include "dis/map.h"        /* 세그먼트 매핑 구조체 */
#include "dis/interrupt.h"  /* 로컬/원격 인터럽트 함수 */
#include "rpc.h"            /* RPC 내부 함수 (_nvm_rpc_handle_insert 등) */
#include "regs.h"           /* NVMe 레지스터 접근 */
#include "ctrl.h"           /* struct controller, _nvm_ctrl_get 등 */
#include "dprintf.h"        /* 디버그 출력 매크로 */
#include <sisci_types.h>    /* SISCI 타입 정의 */
#include <sisci_error.h>    /* SISCI 에러 코드 */
#include <sisci_api.h>      /* SISCI API 함수 선언 */
#include <dis/dis_types.h>  /* DIS_MAX_NSCIS 등 DIS 상수 */


/* RPC 명령의 응답 대기 타임아웃 (밀리초) */
#define RPC_COMMAND_TIMEOUT     2500
/* RPC 핸들 매직 서명: 유효한 핸들인지 확인하는 데 사용 */
#define RPC_MAGIC_SIGNATURE     0xDEADBEEF
/* DIS 어댑터 최대 개수 */
#define N_ADAPTERS              DIS_MAX_NSCIS


/*
 * rpc_cmd - RPC 명령 메시지 형식
 *
 * 클라이언트가 서버에 보내는 요청 메시지 구조체이다.
 * 데이터 인터럽트를 통해 직렬화되어 전송된다.
 * packed 속성으로 구조체 패딩을 제거하여 네트워크 전송에 안전하게 한다.
 */
struct __attribute__((packed)) rpc_cmd
{
    uint32_t                    node_id;    /* 요청을 보내는 클라이언트의 DIS 노드 식별자 */
    uint32_t                    intr_no;    /* 클라이언트의 응답용 인터럽트 번호 (서버가 여기로 응답을 보냄) */
    unsigned char               cmd[64];    /* NVMe Admin 명령 데이터 (nvm_cmd_t, 64바이트) */
};



/*
 * rpc_cpl - RPC 완료 메시지 형식
 *
 * 서버가 클라이언트에게 보내는 응답 메시지 구조체이다.
 * 수정된 명령과 완료 상태를 함께 반환한다.
 */
struct __attribute__((packed)) rpc_cpl
{
    unsigned char               cmd[64];    /* 서버가 수정한 명령 데이터 (0으로 채워지면 명령이 거부된 것) */
    unsigned char               cpl[16];    /* NVMe 완료 엔트리 (nvm_cpl_t, 16바이트) */
};



/*
 * handle_info - RPC 핸들 정보 (디바이스 공유 메모리에 export됨)
 *
 * 서버가 디바이스의 공유 메모리 세그먼트에 기록하는 정보이다.
 * 클라이언트는 이 정보를 읽어 서버의 인터럽트에 연결한다.
 * 각 DIS 어댑터마다 하나의 handle_info 슬롯이 존재한다.
 */
struct __attribute__((packed)) handle_info
{
    uint32_t                    magic;      /* 매직 서명: RPC_MAGIC_SIGNATURE이면 유효한 핸들 */
    uint32_t                    node_id;    /* 서버의 DIS 노드 식별자 */
    uint32_t                    intr_no;    /* 서버의 데이터 인터럽트 번호 */
};



/*
 * binding_handle - 서버 측 RPC 바인딩 핸들
 *
 * NVMe 디바이스를 소유한 노드(서버)가 원격 RPC 요청을 수신하고
 * 처리하기 위한 컨텍스트이다. 데이터 인터럽트로 명령을 수신하고,
 * 로컬 Admin Queue에서 실행한 뒤 결과를 인터럽트로 돌려보낸다.
 */
struct binding_handle
{
    struct controller*          ctrl;       /* NVM 컨트롤러 참조 */
    sci_remote_segment_t        segment;    /* 디바이스 공유 메모리 세그먼트 (핸들 정보 저장용) */
    nvm_aq_ref                  rpc_ref;    /* Admin Queue 참조 (명령 실행용) */
    nvm_dis_rpc_cb_t            rpc_cb;     /* 사용자 필터 콜백 (명령 수락/거부 결정) */
    struct local_intr           intr;       /* 명령 수신용 로컬 데이터 인터럽트 */
};



/*
 * binding - 클라이언트 측 RPC 바인딩
 *
 * 원격 NVMe 디바이스에 RPC 명령을 보내기 위한 클라이언트 컨텍스트이다.
 * 서버에 명령을 보내는 원격 인터럽트와, 응답을 받는 로컬 인터럽트를 관리한다.
 */
struct binding
{
    sci_remote_segment_t        segment;    /* 디바이스 공유 메모리 세그먼트 (서버 핸들 정보 읽기용) */
    struct local_intr           lintr;      /* 응답 수신용 로컬 데이터 인터럽트 */
    struct remote_intr          rintr;      /* 서버에 명령 전송용 원격 데이터 인터럽트 */
};



/*
 * handle_remote_command - 원격 RPC 명령을 수신하여 처리하는 서버 측 핸들러
 *
 * 데이터 인터럽트 콜백으로 호출된다. 수신된 명령을 검증하고,
 * 사용자 필터 콜백이 있으면 명령 수락 여부를 판단한 뒤,
 * 로컬 Admin Queue에서 명령을 실행하고 결과를 클라이언트에 반환한다.
 *
 * @handle:  서버 바인딩 핸들
 * @request: 수신된 RPC 명령 메시지
 * @len:     수신 데이터 길이
 */
static void handle_remote_command(struct binding_handle* handle, struct rpc_cmd* request, uint16_t len)
{
    struct rpc_cpl reply;                /* 클라이언트에 보낼 응답 */

    uint32_t adapter = handle->intr.adapter;  /* 이 핸들이 바인딩된 어댑터 */
    uint32_t node_id = request->node_id;      /* 요청을 보낸 클라이언트의 노드 ID */
    uint32_t intr_no = request->intr_no;      /* 클라이언트의 응답 수신용 인터럽트 번호 */

    /* 수신 데이터 크기가 RPC 명령 구조체와 일치하는지 확인 */
    if (len != sizeof(struct rpc_cmd))
    {
        dprintf("Got unexpected data in RPC binding handle\n");
        return;
    }

    /* 사용자 필터 콜백이 없거나 콜백이 true를 반환한 경우 명령을 실행한다.
     * 콜백은 보안 필터링이나 명령 수정에 사용될 수 있다. */
    if ( handle->rpc_cb == NULL || handle->rpc_cb((nvm_cmd_t*) &request->cmd, adapter, node_id) )
    {
        /* _nvm_local_admin: 로컬 Admin Queue에서 NVMe 명령을 실행한다.
         * 명령 실행 결과(완료 엔트리)가 reply.cpl에 저장된다. */
        _nvm_local_admin(handle->rpc_ref, (const nvm_cmd_t*) &request->cmd, (nvm_cpl_t*) &reply.cpl);
        /* 실행된 명령을 응답에 복사한다 (서버가 명령을 수정했을 수 있음) */
        memcpy(&reply.cmd, &request->cmd, sizeof(nvm_cmd_t));
    }
    else
    {
        /* 사용자 콜백이 명령을 거부한 경우: 명령을 0으로 채워서 거부 신호를 보낸다 */
        memset(&reply.cmd, 0, sizeof(nvm_cmd_t));
    }

    /* 클라이언트의 인터럽트로 응답을 전송한다.
     * fire_and_forget: 연결 → 전송 → 연결 해제를 한번에 수행하는 일회성 전송 */
    int status = _nvm_remote_intr_fire_and_forget(adapter, node_id, intr_no, &reply, sizeof(reply));
    if (status != 0)
    {
        dprintf("Failed to establish reverse connection: %s\n", strerror(errno));
    }
}



/*
 * remote_command - 클라이언트에서 원격 서버에 RPC 명령을 보내고 응답을 기다린다.
 *
 * 클라이언트 측 RPC 호출의 핵심 함수이다:
 * 1. 요청 메시지를 구성하여 서버의 인터럽트에 전송
 * 2. 로컬 인터럽트에서 응답이 도착할 때까지 블로킹 대기
 * 3. 응답에서 수정된 명령과 완료 엔트리를 추출
 *
 * @binding: 클라이언트 바인딩 구조체
 * @cmd:     실행할 NVMe 명령 (응답 시 수정된 명령으로 덮어쓰여짐)
 * @cpl:     명령 완료 엔트리를 받을 포인터
 * @return:  NVM_ERR_PACK으로 패킹된 에러 코드
 */
static int remote_command(struct binding* binding, nvm_cmd_t* cmd, nvm_cpl_t* cpl)
{
    struct rpc_cmd request;
    struct rpc_cpl reply;

    if (cmd == NULL || cpl == NULL)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    /* RPC 요청 메시지를 구성한다 */
    request.node_id = binding->lintr.node_id;  /* 이 클라이언트의 노드 ID */
    request.intr_no = binding->lintr.intr_no;  /* 응답을 받을 인터럽트 번호 */
    memcpy(&request.cmd, cmd, sizeof(nvm_cmd_t));  /* NVMe 명령 데이터 복사 */

    /* 서버의 원격 인터럽트를 트리거하여 RPC 요청을 전송한다 */
    int status = _nvm_remote_intr_trigger(&binding->rintr, &request, sizeof(request));
    if (status != 0)
    {
        return NVM_ERR_PACK(NULL, status);
    }

    /* XXX: 여기서 경쟁 조건이 발생할 수 있는가?
     * XXX: 콜백 모드 인터럽트를 생성하고 조건 변수로 대기하는 것이 더 나을 수 있음 */

    /* 서버의 응답을 타임아웃 내에 수신할 때까지 블로킹 대기한다 */
    status = _nvm_local_intr_wait(&binding->lintr, &reply, sizeof(reply), RPC_COMMAND_TIMEOUT);
    if (status != 0)
    {
        return NVM_ERR_PACK(NULL, status);
    }

    /* 응답에서 수정된 명령과 완료 엔트리를 클라이언트 버퍼로 복사한다 */
    memcpy(cmd, &reply.cmd, sizeof(nvm_cmd_t));
    memcpy(cpl, &reply.cpl, sizeof(nvm_cpl_t));

    return NVM_ERR_PACK(NULL, 0);        /* 성공 */
}



/*
 * write_handle_info - RPC 핸들 정보를 디바이스 공유 메모리에 기록한다.
 *
 * 서버의 인터럽트 정보(노드 ID, 인터럽트 번호, 매직 서명)를
 * 디바이스의 공유 메모리 세그먼트에 기록하여 클라이언트가 검색할 수 있게 한다.
 * clear=true이면 해당 슬롯을 0으로 초기화하여 핸들을 무효화한다.
 *
 * @handle:  서버 바인딩 핸들
 * @adapter: 정보를 기록할 어댑터 슬롯 인덱스
 * @clear:   true이면 핸들 정보를 지운다, false이면 기록한다
 * @return:  성공 시 0, 실패 시 EIO
 */
static int write_handle_info(const struct binding_handle* handle, uint32_t adapter, bool clear)
{
    sci_error_t err;
    sci_map_t map;
    /* 공유 세그먼트에 어댑터 수만큼의 handle_info 슬롯이 배열로 존재한다 */
    const size_t size = sizeof(struct handle_info) * N_ADAPTERS;
    volatile struct handle_info* infos;

    /* SCIMapRemoteSegment: 디바이스 공유 메모리 세그먼트를 가상 주소에 매핑하여
     * CPU에서 직접 읽고 쓸 수 있게 한다. WC 모드(기본)로 매핑된다. */
    infos = (volatile struct handle_info*) SCIMapRemoteSegment(handle->segment, &map, 0, size, NULL, 0, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to map shared device segment: %s\n", _SCIGetErrorString(err));
        return EIO;
    }

    /* 핸들 정보를 구성한다 */
    struct handle_info info;
    info.magic = clear ? 0 : RPC_MAGIC_SIGNATURE;    /* clear이면 매직을 0으로 → 무효 */
    info.node_id = clear ? 0 : handle->intr.node_id; /* 서버 노드 ID */
    info.intr_no = clear ? 0 : handle->intr.intr_no; /* 서버 인터럽트 번호 */

    /* 해당 어댑터 슬롯에 정보를 기록한다 */
    infos[adapter] = info;

    /* nvm_wcb_flush: Write-Combining 버퍼를 플러시하여
     * 기록한 데이터가 즉시 원격에서 보이도록 보장한다.
     * WC 매핑은 쓰기를 버퍼링하므로 명시적 플러시가 필요하다. */
    nvm_wcb_flush();

    /* 매핑을 해제한다 */
    do
    {
        SCIUnmapSegment(map, 0, &err);
    }
    while (err == SCI_ERR_BUSY);

    return 0;
}



/*
 * try_create - 디바이스 공유 메모리 세그먼트를 생성하거나 기존 것에 연결한다.
 *
 * RPC 핸들 정보를 저장할 디바이스의 공유 메모리 세그먼트에 연결을 시도한다.
 * 세그먼트가 아직 없으면 SCICreateDeviceSegment()로 생성한 뒤 연결한다.
 * 이 노드가 세그먼트의 소유자인지도 확인한다.
 *
 * @segment: 연결된 원격 세그먼트 핸들을 받을 포인터
 * @dev:     SmartIO 디바이스 핸들
 * @return:  성공 시 0, 소유자가 아니면 EEXIST, 기타 실패 시 적절한 errno
 */
static int try_create(sci_remote_segment_t* segment, const struct device* dev)
{
    int status;
    sci_remote_segment_t rseg;
    unsigned adapter = 0;

    *segment = NULL;

    /* 먼저 기존 공유 세그먼트에 연결을 시도한다 */
    status = _nvm_device_memory_get(&rseg, dev, 0, SCI_MEMTYPE_SHARED);
    if (status != 0)
    {
        /* 세그먼트가 없으면 새로 생성한다 */
        sci_error_t err;

        /* SCICreateDeviceSegment: 디바이스의 공유 메모리에 세그먼트를 생성한다.
         * SCI_MEMTYPE_SHARED: 여러 호스트가 접근 가능한 공유 메모리 유형
         * SCI_MEMACCESS_HOST_WRITE | SCI_MEMACCESS_MULTIHOST_READ:
         *   소유자 호스트는 쓰기 가능, 다른 호스트는 읽기만 가능하도록 설정 */
        SCICreateDeviceSegment(dev->device, 0, sizeof(struct handle_info) * N_ADAPTERS,
                SCI_MEMTYPE_SHARED, SCI_MEMACCESS_HOST_WRITE | SCI_MEMACCESS_MULTIHOST_READ, 0, &err);

        switch (err)
        {
            case SCI_ERR_SEGMENTID_USED: /* 세그먼트가 이미 존재함 (다른 노드가 생성) */
            case SCI_ERR_OK:             /* 세그먼트 생성 성공 */
                break;

            default:
                dprintf("Unexpected error when creating shared device segment: %s\n", _SCIGetErrorString(err));
                return EIO;
        }

        /* 생성(또는 이미 존재하는) 세그먼트에 연결한다 */
        status = _nvm_device_memory_get(&rseg, dev, 0, SCI_MEMTYPE_SHARED);
        if (status != 0)
        {
            return status;
        }
    }

    /* 이 노드가 세그먼트의 소유자인지 확인한다.
     * 세그먼트를 호스팅하는 노드만이 핸들 정보를 기록할 수 있다. */
    for (adapter = 0; adapter < N_ADAPTERS; ++adapter)
    {
        sci_error_t err;
        uint32_t node_id = 0;

        /* 각 어댑터의 로컬 노드 ID를 조회하여 세그먼트 노드 ID와 비교한다 */
        SCIGetLocalNodeId((unsigned int) adapter, &node_id, 0, &err);
        if (err == SCI_ERR_OK)
        {
            /* SCIGetRemoteSegmentNodeId: 원격 세그먼트가 위치한 노드 ID를 반환한다 */
            if (SCIGetRemoteSegmentNodeId(rseg) == node_id)
            {
                break;                   /* 일치: 이 노드가 소유자 */
            }
        }
    }

    if (adapter == N_ADAPTERS)
    {
        return EEXIST;                   /* 모든 어댑터를 확인했으나 소유자가 아님 */
    }

    *segment = rseg;
    return 0;
}



/*
 * create_binding_handle - 서버 측 RPC 바인딩 핸들을 생성한다.
 *
 * NVMe 디바이스를 소유한 노드에서 호출되어 원격 RPC 요청을 수신할 준비를 한다:
 * 1. 컨트롤러 참조 획득
 * 2. 디바이스 공유 메모리 세그먼트 생성/연결
 * 3. 로컬 데이터 인터럽트 생성 (명령 수신용, 콜백 모드)
 * 4. 핸들 정보를 공유 메모리에 기록
 *
 * @handle:  생성된 바인딩 핸들을 받을 이중 포인터
 * @ref:     Admin Queue 참조
 * @adapter: 사용할 DIS 어댑터 번호
 * @cb:      명령 필터 콜백 (NULL이면 모든 명령 수락)
 * @return:  성공 시 0, 실패 시 errno 값
 */
static int create_binding_handle(struct binding_handle** handle, nvm_aq_ref ref, uint32_t adapter, nvm_dis_rpc_cb_t cb)
{
    int status;
    *handle = NULL;
    sci_remote_segment_t rseg;

    /* Admin Queue 참조에서 NVMe 컨트롤러 핸들을 추출한다 */
    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(ref);
    if (ctrl == NULL)
    {
        return EINVAL;
    }

    /* 컨트롤러 참조를 획득한다 (참조 카운트 증가) */
    struct controller* cref = _nvm_ctrl_get(ctrl);
    if (cref == NULL)
    {
        return ENOTTY;
    }

    /* 디바이스 공유 메모리 세그먼트를 생성하거나 기존 것에 연결한다 */
    status = try_create(&rseg, cref->device);
    if (status != 0)
    {
        _nvm_ctrl_put(cref);
        return status;
    }

    /* 바인딩 핸들 구조체를 힙에 할당한다 */
    struct binding_handle* bh = (struct binding_handle*) malloc(sizeof(struct binding_handle));
    if (bh == NULL)
    {
        _nvm_device_memory_put(&rseg);
        _nvm_ctrl_put(cref);
        dprintf("Failed to allocate RPC binding handle: %s\n", strerror(errno));
        return ENOMEM;
    }

    bh->ctrl = cref;                    /* 컨트롤러 참조 저장 */
    bh->segment = rseg;                 /* 공유 메모리 세그먼트 저장 */
    bh->rpc_ref = ref;                  /* Admin Queue 참조 저장 (명령 실행 시 사용) */
    bh->rpc_cb = cb;                    /* 명령 필터 콜백 저장 */

    /* 로컬 데이터 인터럽트를 생성한다 (콜백 모드).
     * 원격 클라이언트가 이 인터럽트를 트리거하면 handle_remote_command 콜백이 호출된다.
     * bh를 콜백 데이터로 전달하여 핸들러에서 컨텍스트에 접근할 수 있게 한다. */
    status = _nvm_local_intr_get(&bh->intr, adapter, bh, (intr_callback_t) handle_remote_command);
    if (status != 0)
    {
        _nvm_device_memory_put(&bh->segment);
        _nvm_ctrl_put(bh->ctrl);
        free(bh);
        return status;
    }

    /* 핸들 정보(노드 ID, 인터럽트 번호)를 디바이스 공유 메모리에 기록한다.
     * 클라이언트 노드는 이 정보를 읽어 서버 인터럽트에 연결한다. */
    status = write_handle_info(bh, adapter, false);
    if (status != 0)
    {
        _nvm_device_memory_put(&bh->segment);
        _nvm_ctrl_put(bh->ctrl);
        free(bh);
        return status;
    }

    *handle = bh;
    return 0;
}



/*
 * remove_binding_handle - 서버 측 RPC 바인딩 핸들을 제거한다.
 *
 * 공유 메모리의 핸들 정보를 지우고, 인터럽트와 세그먼트를 해제하고,
 * 컨트롤러 참조를 반환한다.
 *
 * @adapter: 핸들이 바인딩된 어댑터 번호
 * @handle:  제거할 바인딩 핸들
 */
static void remove_binding_handle(uint32_t adapter, struct binding_handle* handle)
{
    /* 공유 메모리에서 핸들 정보를 0으로 지운다 (clear=true) */
    write_handle_info(handle, adapter, true);

    /* 로컬 데이터 인터럽트를 제거한다 */
    _nvm_local_intr_put(&handle->intr);
    /* 디바이스 공유 메모리 세그먼트 연결을 해제한다 */
    _nvm_device_memory_put(&handle->segment);
    /* 컨트롤러 참조를 반환한다 */
    _nvm_ctrl_put(handle->ctrl);

    /* 핸들 메모리를 해제한다 */
    free(handle);
}



/*
 * try_bind - 디바이스 공유 메모리에서 서버 핸들 정보를 읽고 원격 인터럽트에 연결한다.
 *
 * 클라이언트가 서버에 연결하기 위해 호출한다.
 * 공유 메모리의 handle_info 배열을 순회하며 유효한(매직 서명이 일치하는)
 * 핸들을 찾아 해당 서버의 인터럽트에 연결을 시도한다.
 *
 * @binding: 클라이언트 바인딩 구조체
 * @adapter: DIS 어댑터 번호
 * @max:     검색할 최대 슬롯 수 (= N_ADAPTERS)
 * @return:  성공 시 0, 실패 시 ECONNREFUSED
 */
static int try_bind(struct binding* binding, uint32_t adapter, unsigned max)
{
    sci_error_t err;
    sci_map_t map;
    volatile const struct handle_info* info;

    /* 디바이스 공유 메모리 세그먼트를 읽기 전용으로 매핑한다 */
    info = SCIMapRemoteSegment(binding->segment, &map, 0, sizeof(struct handle_info) * max, NULL, SCI_FLAG_READONLY_MAP, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to map remote segment: %s\n", _SCIGetErrorString(err));
        return EIO;
    }

    /* 모든 어댑터 슬롯을 순회하며 유효한 핸들을 검색한다 */
    for (unsigned i = 0; i < max; ++i)
    {
        /* handle_info 슬롯에서 정보를 읽는다 */
        uint32_t magic = info[i].magic;
        uint32_t node_id = info[i].node_id;
        uint32_t intr_no = info[i].intr_no;

        /* 매직 서명이 일치하지 않으면 빈 슬롯이므로 건너뛴다 */
        if (magic != RPC_MAGIC_SIGNATURE)
        {
            continue;
        }

        /* 유효한 슬롯을 찾으면 해당 서버의 원격 인터럽트에 연결을 시도한다 */
        if (_nvm_remote_intr_get(&binding->rintr, adapter, node_id, intr_no) == 0)
        {
            /* 연결 성공: 매핑을 해제하고 반환 */
            SCIUnmapSegment(map, 0, &err);
            return 0;
        }
    }

    /* 유효한 핸들을 찾지 못했거나 모든 연결 시도가 실패 */
    SCIUnmapSegment(map, 0, &err);
    return ECONNREFUSED;
}


/*
 * create_binding - 클라이언트 측 RPC 바인딩을 생성한다.
 *
 * 원격 NVMe 디바이스의 공유 메모리에서 서버 핸들 정보를 읽고,
 * 응답 수신용 로컬 인터럽트를 생성하고,
 * 서버의 원격 인터럽트에 연결한다.
 *
 * @handle:  생성된 바인딩을 받을 이중 포인터
 * @dev:     SmartIO 디바이스 핸들
 * @adapter: DIS 어댑터 번호
 * @return:  성공 시 0, 실패 시 errno 값
 */
static int create_binding(struct binding** handle, const struct device* dev, uint32_t adapter)
{
    int status;
    *handle = NULL;

    /* 바인딩 구조체를 힙에 할당한다 */
    struct binding* binding = (struct binding*) malloc(sizeof(struct binding));
    if (binding == NULL)
    {
        dprintf("Failed to allocate binding descriptor: %s\n", strerror(errno));
        return ENOMEM;
    }

    /* 디바이스의 공유 메모리 세그먼트에 연결한다.
     * 서버가 기록한 핸들 정보를 읽기 위해 SCI_MEMTYPE_SHARED로 연결한다. */
    status = _nvm_device_memory_get(&binding->segment, dev, 0, SCI_MEMTYPE_SHARED);
    if (status != 0)
    {
        free(binding);
        return status;                   /* 세그먼트가 없으면 서버가 아직 준비되지 않은 것 */
    }

    /* 응답 수신용 로컬 데이터 인터럽트를 생성한다 (콜백 없이 대기 모드).
     * cb_data=NULL, cb=NULL이므로 SCIWaitForDataInterrupt()로 응답을 대기하게 된다. */
    status = _nvm_local_intr_get(&binding->lintr, adapter, NULL, NULL);
    if (status != 0)
    {
        _nvm_device_memory_put(&binding->segment);
        free(binding);
        return status;
    }

    /* 공유 메모리에서 서버 핸들 정보를 읽고 서버의 인터럽트에 연결한다 */
    status = try_bind(binding, adapter, N_ADAPTERS);
    if (status != 0)
    {
        _nvm_local_intr_put(&binding->lintr);
        _nvm_device_memory_put(&binding->segment);
        free(binding);
        return status;
    }

    *handle = binding;
    return 0;
}



/*
 * remove_binding - 클라이언트 측 RPC 바인딩을 제거한다.
 *
 * 원격 인터럽트 연결, 로컬 인터럽트, 공유 메모리 연결을 순서대로 해제한다.
 *
 * @binding: 제거할 바인딩 구조체
 */
static void remove_binding(struct binding* binding)
{
    /* 서버의 원격 인터럽트 연결을 해제한다 */
    _nvm_remote_intr_put(&binding->rintr);
    /* 응답 수신용 로컬 인터럽트를 제거한다 */
    _nvm_local_intr_put(&binding->lintr);
    /* 디바이스 공유 메모리 연결을 해제한다 */
    _nvm_device_memory_put(&binding->segment);

    /* 바인딩 메모리를 해제한다 */
    free(binding);
}



/*
 * nvm_dis_rpc_enable - 지정된 어댑터에서 RPC 서버를 활성화한다.
 *
 * NVMe 디바이스를 소유한 노드에서 호출하여 원격 Admin 명령 수신을 시작한다.
 * RPC 핸들을 생성하고 내부 핸들 테이블에 등록한다.
 *
 * @ref:     Admin Queue 참조
 * @adapter: RPC를 활성화할 DIS 어댑터 번호
 * @filter:  명령 필터 콜백 (NULL이면 모든 명령 수락)
 * @return:  성공 시 0, 실패 시 errno 값
 */
int nvm_dis_rpc_enable(nvm_aq_ref ref, uint32_t adapter, nvm_dis_rpc_cb_t filter)
{
    int err;
    struct binding_handle* handle;

    /* 어댑터 번호가 유효 범위 내인지 확인 */
    if (adapter >= N_ADAPTERS)
    {
        return EINVAL;
    }

    /* 서버 바인딩 핸들을 생성한다 */
    err = create_binding_handle(&handle, ref, adapter, filter);
    if (err != 0)
    {
        return err;
    }

    /* 생성된 핸들을 내부 RPC 핸들 테이블에 등록한다.
     * 해제 시 remove_binding_handle이 호출되도록 콜백을 등록한다. */
    err = _nvm_rpc_handle_insert(ref, adapter, handle, (rpc_free_handle_t) remove_binding_handle);
    if (err != 0)
    {
        remove_binding_handle(adapter, handle);  /* 등록 실패 시 핸들 정리 */
        return err;
    }

    return 0;
}



/*
 * nvm_dis_rpc_disable - 지정된 어댑터에서 RPC 서버를 비활성화한다.
 *
 * 내부 핸들 테이블에서 해당 어댑터의 핸들을 제거한다.
 * 제거 시 등록된 콜백(remove_binding_handle)이 자동 호출되어 자원이 정리된다.
 *
 * @ref:     Admin Queue 참조
 * @adapter: RPC를 비활성화할 DIS 어댑터 번호
 */
void nvm_dis_rpc_disable(nvm_aq_ref ref, uint32_t adapter)
{
    /* 핸들 테이블에서 해당 어댑터의 RPC 핸들을 제거한다 */
    _nvm_rpc_handle_remove(ref, adapter);
}



/*
 * nvm_dis_rpc_bind - 원격 NVMe 디바이스에 RPC 클라이언트로 바인딩한다.
 *
 * 원격 노드의 NVMe 디바이스에 Admin 명령을 보내기 위한 RPC 연결을 수립한다.
 * 반환된 핸들(nvm_aq_ref)을 통해 nvm_admin_command() 등으로 명령을 실행할 수 있다.
 *
 * @handle:  생성된 Admin Queue 참조를 받을 포인터
 * @ctrl:    원격 NVMe 컨트롤러 핸들
 * @adapter: 사용할 DIS 어댑터 번호
 * @return:  성공 시 0, 실패 시 errno 값
 */
int nvm_dis_rpc_bind(nvm_aq_ref* handle, const nvm_ctrl_t* ctrl, uint32_t adapter)
{
    nvm_aq_ref ref;
    *handle = NULL;

    /* Admin Queue 참조를 생성한다 */
    int err = _nvm_ref_get(&ref, ctrl);
    if (err != 0)
    {
        return err;
    }

    /* 원격 서버에 RPC 바인딩을 생성한다 */
    struct binding* binding;
    err = create_binding(&binding, _nvm_container_of(ctrl, struct controller, handle)->device, adapter);
    if (err != 0)
    {
        _nvm_ref_put(ref);               /* 실패 시 참조 해제 */
        return err;
    }

    /* RPC 바인딩을 참조에 등록한다.
     * remote_command가 RPC stub으로 등록되어, 이후 Admin 명령 실행 시
     * 자동으로 원격 서버에 전달된다. */
    err = _nvm_rpc_bind(ref, binding, (rpc_free_binding_t) remove_binding, (rpc_stub_t) remote_command);
    if (err != 0)
    {
        remove_binding(binding);          /* 바인딩 정리 */
        _nvm_ref_put(ref);               /* 참조 해제 */
        return err;
    }

    *handle = ref;                        /* 성공: RPC 연결된 Admin Queue 참조 반환 */
    return 0;
}
