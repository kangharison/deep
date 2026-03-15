# 18. CPU MMU Page Fault Tracing (mmiotrace) vs IOMMU Page Fault Tracing: 대칭 구조 분석

> **대상 아키텍처**: x86_64 (Intel VT-d IOMMU 기준)
> **용어 주의**: 이 문서에서 "ARM/DISARM"은 ARM 아키텍처가 아니라 "무장/해제(트랩 설치/해제)"를 의미한다. 커널 함수명 `arm_kmmio_fault_page()` / `disarm_kmmio_fault_page()`에서 유래한 용어이다.

## 1. 대칭 구조: 같은 패턴의 두 가지 적용

CPU MMU 기반 MMIO tracing과 IOMMU 기반 DMA tracing은 놀랍도록 동일한 설계 패턴을 사용한다.
두 메커니즘 모두 "페이지 테이블에서 매핑을 제거 → fault 유발 → 가로채서 기록 → 매핑 복구 → 재무장"이라는 동일한 사이클을 따른다.

```
  CPU → Device (MMIO tracing):              Device → Host (DMA tracing):
  ============================              ============================
  CPU MMU 페이지 테이블                      IOMMU 페이지 테이블
       │                                         │
  PTE에서 Present bit 제거                   IOMMU PTE에서 매핑 제거
  (clear_page_presence)                      (iommu_unmap)
       │                                         │
  CPU가 해당 주소 접근                       디바이스가 해당 주소 DMA
  (MMIO read/write)                          (NVMe CQ write 등)
       │                                         │
  CPU Page Fault (#PF)                       IOMMU DMA Remapping Fault
  (do_page_fault → kmmio_handler)            (dmar_fault / prq_event_thread)
       │                                         │
  kmmio_handler() 호출                       dmar_fault() / iopf_handler() 호출
       │                                         │
  접근 정보 기록                             접근 정보 기록
  (pre_handler 콜백)                         (dmar_fault_do_one / intel_prq_report)
       │                                         │
  PTE 복구 (Present bit 복원)                IOMMU PTE 복구 (매핑 복원)
  (disarm_kmmio_fault_page)                  (iommu_map)
       │                                         │
  Single-step (TF flag)                      Page Response (PRI) 또는 재시도
  (X86_EFLAGS_TF 설정)                       (iopf_group_response / qi_submit_sync)
       │                                         │
  실행 완료 후 다시 PTE 제거                  DMA 완료 후 다시 매핑 제거
  (post_kmmio_handler → re-arm)              (응답 후 re-unmap)
```

### 핵심 대칭: 같은 4단계 사이클

```
┌─────────────────────────────────────────────────────────────┐
│                     공통 패턴 (4단계)                        │
│                                                             │
│  ① ARM (무장)                                               │
│     페이지 테이블에서 매핑/존재 비트를 제거                  │
│     → 해당 주소 접근 시 fault 발생하도록 트랩 설치           │
│                                                             │
│  ② FAULT (포착)                                             │
│     접근 시도가 fault를 유발                                 │
│     → 핸들러에서 접근 정보(주소, R/W, 발생 주체) 기록        │
│                                                             │
│  ③ DISARM + EXECUTE (해제 + 실행)                           │
│     매핑을 복구하여 접근이 성공하도록 허용                   │
│     → 해당 접근이 실제로 완료됨                              │
│                                                             │
│  ④ RE-ARM (재무장)                                          │
│     접근 완료 후 다시 매핑을 제거                            │
│     → 다음 접근도 추적 가능                                  │
└─────────────────────────────────────────────────────────────┘
```

이 패턴은 소프트웨어 breakpoint/watchpoint와 본질적으로 같은 원리이다.
차이점은 "누가 접근하는가"(CPU vs 디바이스)와 "어떤 페이지 테이블을 조작하는가"(CPU MMU vs IOMMU)뿐이다.


## 2. CPU MMU 기반 MMIO Tracing (mmiotrace) 구현 상세

### 2.1 전체 흐름

```
mmiotrace 활성화
     │
     ▼
ioremap() 호출 감지
     │
     ▼
mmiotrace_ioremap()
     │
     ▼
ioremap_trace_core()
     │
     ├─ register_kmmio_probe() ─── 프로브 등록
     │       │
     │       ▼
     │  add_kmmio_fault_page()
     │       │
     │       ▼
     │  arm_kmmio_fault_page() ── ① ARM
     │       │
     │       ▼
     │  clear_page_presence(f, true)
     │       │
     │       ├─ clear_pte_presence(): PTE를 0으로 설정
     │       │
     │       └─ flush_tlb_one_kernel(): TLB 무효화
     │
     ▼
  (CPU가 MMIO 주소에 접근)
     │
     ▼
  Page Fault (#PF) 발생 ──────── ② FAULT
     │
     ▼
  do_page_fault()
     │
     ▼
  kmmio_handler(regs, addr)
     │
     ├─ get_kmmio_fault_page(): 무장된 페이지인지 확인
     │
     ├─ pre_handler(probe, regs, addr): 접근 정보 기록
     │       │
     │       ▼
     │   pre() in mmio-mod.c:
     │   - get_ins_type(IP): 읽기/쓰기 판별
     │   - get_ins_mem_width(IP): 접근 크기 (1/2/4/8 바이트)
     │   - get_ins_reg_val/imm_val(): 쓰기 값 추출
     │
     ├─ disarm_kmmio_fault_page() ── ③ DISARM
     │       │
     │       ▼
     │   clear_page_presence(f, false): PTE 복원
     │
     └─ regs->flags |= X86_EFLAGS_TF: single-step 모드 설정
        regs->flags &= ~X86_EFLAGS_IF: 인터럽트 비활성화
     │
     ▼
  (MMIO 접근 명령어 1개 실행)
     │
     ▼
  Debug Trap (#DB) 발생
     │
     ▼
  kmmio_die_notifier() → post_kmmio_handler() ── ④ RE-ARM
     │
     ├─ post_handler(probe, condition, regs): 읽기 값 기록
     │       │
     │       ▼
     │   post() in mmio-mod.c:
     │   - REG_READ인 경우 get_ins_reg_val(IP, regs)로 읽기 값 추출
     │   - mmio_trace_rw(): trace 시스템에 기록
     │
     ├─ arm_kmmio_fault_page(): 다시 PTE에서 Present 제거
     │
     └─ regs->flags 복원: TF 해제, IF 복원
```

### 2.2 핵심 데이터 구조 (kmmio.c)

```c
/* arch/x86/mm/kmmio.c */

struct kmmio_fault_page {
    struct list_head list;
    struct kmmio_fault_page *release_next;
    unsigned long addr;           /* 무장된 페이지의 가상 주소 */
    pteval_t old_presence;        /* 무장 전 원래 PTE 값 저장 */
    bool armed;                   /* 현재 무장 상태인지 */
    int count;                    /* 이 페이지를 참조하는 프로브 수 */
    bool scheduled_for_release;   /* 해제 예약 상태 */
};

struct kmmio_context {
    struct kmmio_fault_page *fpage;   /* 현재 처리 중인 fault 페이지 */
    struct kmmio_probe *probe;        /* 해당 프로브 */
    unsigned long saved_flags;        /* 원래 EFLAGS (TF, IF) */
    unsigned long addr;               /* fault 발생 주소 */
    int active;                       /* 활성 상태 (0 또는 1) */
};

/* Per-CPU context: 각 CPU는 한 번에 하나의 kmmio만 처리 */
static DEFINE_PER_CPU(struct kmmio_context, kmmio_ctx);

/* 해시 테이블: fault 주소로 kmmio_fault_page를 빠르게 검색 */
#define KMMIO_PAGE_HASH_BITS 4
#define KMMIO_PAGE_TABLE_SIZE (1 << KMMIO_PAGE_HASH_BITS)  /* = 16 */
static struct list_head kmmio_page_table[KMMIO_PAGE_TABLE_SIZE];
```

### 2.3 코드 분석: Step 1 - 페이지 무장 (arm)

#### clear_pte_presence() - PTE에서 Present 비트 제거

```c
/* arch/x86/mm/kmmio.c:147 */
static void clear_pte_presence(pte_t *pte, bool clear, pteval_t *old)
{
    pteval_t v = pte_val(*pte);   /* 현재 PTE 값 읽기 */
    if (clear) {
        *old = v;                  /* 원래 값을 백업해둠 (나중에 복원용) */
        /* PTE를 0으로 설정 → Present bit = 0, 접근 시 #PF 발생 */
        pte_clear(&init_mm, 0, pte);
    } else {
        /* 복원 모드: 백업해둔 원래 PTE 값으로 되돌림 */
        set_pte_atomic(pte, __pte(*old));
    }
}
```

이 함수가 핵심이다. `clear=true`로 호출하면 PTE를 완전히 제거하여 해당 페이지에 대한 모든 접근이 Page Fault를 유발하도록 만든다. `clear=false`로 호출하면 백업된 PTE를 복원한다.

2MB 대형 페이지도 지원한다:

```c
/* arch/x86/mm/kmmio.c:133 */
static void clear_pmd_presence(pmd_t *pmd, bool clear, pmdval_t *old)
{
    pmd_t new_pmd;
    pmdval_t v = pmd_val(*pmd);
    if (clear) {
        *old = v;
        new_pmd = pmd_mkinvalid(*pmd);   /* PMD를 invalid로 표시 */
    } else {
        new_pmd = __pmd(*old);            /* 원래 PMD 복원 */
    }
    set_pmd(pmd, new_pmd);
}
```

#### clear_page_presence() - 페이지 레벨에 따른 분기

```c
/* arch/x86/mm/kmmio.c:160 */
static int clear_page_presence(struct kmmio_fault_page *f, bool clear)
{
    unsigned int level;
    pte_t *pte = lookup_address(f->addr, &level);
    /* lookup_address(): 가상 주소에 대한 PTE와 페이지 레벨을 찾음 */

    if (!pte) {
        pr_err("no pte for addr 0x%08lx\n", f->addr);
        return -1;
    }

    switch (level) {
    case PG_LEVEL_2M:
        /* 2MB 대형 페이지인 경우 PMD 조작 */
        clear_pmd_presence((pmd_t *)pte, clear, &f->old_presence);
        break;
    case PG_LEVEL_4K:
        /* 일반 4KB 페이지인 경우 PTE 조작 */
        clear_pte_presence(pte, clear, &f->old_presence);
        break;
    default:
        pr_err("unexpected page level 0x%x.\n", level);
        return -1;
    }

    /* TLB에 캐시된 매핑도 무효화해야 함!
     * 그렇지 않으면 CPU가 TLB에서 매핑을 찾아 fault 없이 접근할 수 있다 */
    flush_tlb_one_kernel(f->addr);
    return 0;
}
```

#### arm_kmmio_fault_page() - 무장 실행

```c
/* arch/x86/mm/kmmio.c:197 */
static int arm_kmmio_fault_page(struct kmmio_fault_page *f)
{
    int ret;
    /* 이중 무장 방지 체크 */
    WARN_ONCE(f->armed, KERN_ERR pr_fmt("kmmio page already armed.\n"));
    if (f->armed) {
        pr_warn("double-arm: addr 0x%08lx, ref %d, old %d\n",
                f->addr, f->count, !!f->old_presence);
    }
    /* 핵심: PTE에서 Present bit 제거 */
    ret = clear_page_presence(f, true);
    WARN_ONCE(ret < 0, KERN_ERR pr_fmt("arming at 0x%08lx failed.\n"),
              f->addr);
    f->armed = true;
    return ret;
}
```

#### disarm_kmmio_fault_page() - 무장 해제

```c
/* arch/x86/mm/kmmio.c:213 */
static void disarm_kmmio_fault_page(struct kmmio_fault_page *f)
{
    /* PTE 복원: clear_page_presence(f, false)는 백업된 old_presence로 복구 */
    int ret = clear_page_presence(f, false);
    WARN_ONCE(ret < 0,
              KERN_ERR "kmmio disarming at 0x%08lx failed.\n", f->addr);
    f->armed = false;
}
```

### 2.4 코드 분석: Step 2 - Page Fault 처리

```c
/* arch/x86/mm/kmmio.c:236 */
int kmmio_handler(struct pt_regs *regs, unsigned long addr)
{
    struct kmmio_context *ctx;
    struct kmmio_fault_page *faultpage;
    int ret = 0;
    unsigned long page_base = addr;
    unsigned int l;
    pte_t *pte = lookup_address(addr, &l);

    if (!pte)
        return -EINVAL;
    /* 페이지 경계로 정렬: 4KB면 하위 12비트, 2MB면 하위 21비트 마스크 */
    page_base &= page_level_mask(l);

    /*
     * RCU read lock을 single-step 전체 구간에 걸쳐 유지한다.
     * 이렇게 하면 probe와 fault_page가 중간에 해제되는 것을 방지한다.
     * rcu_read_lock_sched()는 preemption도 비활성화하여
     * single-step 중 다른 프로세스로 전환되는 것을 방지한다.
     */
    rcu_read_lock_sched_notrace();

    /* 이 주소가 우리가 무장한 페이지인지 해시 테이블에서 검색 */
    faultpage = get_kmmio_fault_page(page_base);
    if (!faultpage) {
        /* kmmio가 원인이 아닌 일반 page fault → 처리하지 않음 */
        goto no_kmmio;
    }

    /* Per-CPU context 가져오기 */
    ctx = this_cpu_ptr(&kmmio_ctx);
    if (ctx->active) {
        /*
         * 이미 이 CPU에서 kmmio 처리 중인데 또 fault가 발생.
         * 같은 페이지에 대한 2차 fault면 다른 원인이므로 do_page_fault에 위임.
         * 다른 페이지면 재귀적 probe hit → 위험하므로 disarm.
         */
        if (page_base == ctx->addr) {
            pr_debug("secondary hit for 0x%08lx CPU %d.\n",
                     addr, smp_processor_id());
            if (!faultpage->old_presence)
                pr_info("unexpected secondary hit for address "
                        "0x%08lx on CPU %d.\n", addr, smp_processor_id());
        } else {
            pr_emerg("recursive probe hit on CPU %d, "
                     "for address 0x%08lx. Ignoring.\n",
                     smp_processor_id(), addr);
            disarm_kmmio_fault_page(faultpage);
        }
        goto no_kmmio;
    }
    ctx->active++;   /* 이 CPU에서 kmmio 처리 시작 표시 */

    ctx->fpage = faultpage;
    ctx->probe = get_kmmio_probe(page_base);
    /* 원래 EFLAGS의 TF(Trap Flag)와 IF(Interrupt Flag) 저장 */
    ctx->saved_flags = (regs->flags & (X86_EFLAGS_TF | X86_EFLAGS_IF));
    ctx->addr = page_base;

    /* ★ pre_handler 콜백 호출: MMIO 접근 정보 기록 ★ */
    if (ctx->probe && ctx->probe->pre_handler)
        ctx->probe->pre_handler(ctx->probe, regs, addr);

    /*
     * Single-step 모드 설정:
     * TF=1: 다음 명령어 1개 실행 후 #DB (Debug Exception) 발생
     * IF=0: 인터럽트 비활성화 (single-step 중 인터럽트 금지)
     */
    regs->flags |= X86_EFLAGS_TF;
    regs->flags &= ~X86_EFLAGS_IF;

    /* PTE 복원하여 MMIO 접근이 실제로 수행되도록 함 */
    disarm_kmmio_fault_page(ctx->fpage);

    /*
     * ★ 중요한 SMP 한계 ★
     * PTE가 복원된 순간부터 re-arm될 때까지, 다른 CPU의 MMIO 접근은
     * fault 없이 통과한다. 이 접근은 추적되지 않고 놓친다.
     * 이것이 mmiotrace가 SMP 환경에서 이벤트를 놓칠 수 있는 이유이다.
     */

    return 1; /* fault가 kmmio에 의해 처리됨 */

no_kmmio:
    rcu_read_unlock_sched_notrace();
    return ret;
}
```

### 2.5 코드 분석: Step 3 - Single-step 완료 및 재무장

```c
/* arch/x86/mm/kmmio.c:334 */
static int post_kmmio_handler(unsigned long condition, struct pt_regs *regs)
{
    int ret = 0;
    struct kmmio_context *ctx = this_cpu_ptr(&kmmio_ctx);

    if (!ctx->active) {
        /* 활성 컨텍스트 없이 debug trap 발생 → 외부 원인 */
        pr_warn("unexpected debug trap on CPU %d.\n", smp_processor_id());
        goto out;
    }

    /* ★ post_handler 콜백: MMIO 접근 완료 후 결과 기록 ★
     * 읽기의 경우 여기서 읽은 값을 레지스터에서 추출할 수 있다.
     * (pre_handler 시점에는 아직 MMIO 접근이 실행되지 않았으므로
     *  읽기 값을 알 수 없었다)
     */
    if (ctx->probe && ctx->probe->post_handler)
        ctx->probe->post_handler(ctx->probe, condition, regs);

    /* ★ 재무장: 다시 PTE에서 Present bit 제거 ★ */
    arch_spin_lock(&kmmio_lock);
    if (ctx->fpage->count)
        arm_kmmio_fault_page(ctx->fpage);
    /* count가 0이면 unregister 중이므로 재무장하지 않음 */
    arch_spin_unlock(&kmmio_lock);

    /* EFLAGS 복원: TF 제거, 원래의 TF/IF 값 복원 */
    regs->flags &= ~X86_EFLAGS_TF;
    regs->flags |= ctx->saved_flags;

    /* 활성 상태 해제 */
    ctx->active--;
    BUG_ON(ctx->active);  /* active는 반드시 0이어야 함 */

    /* kmmio_handler()에서 잡았던 RCU read lock 해제 */
    rcu_read_unlock_sched_notrace();

    if (!(regs->flags & X86_EFLAGS_TF))
        ret = 1;  /* debug trap을 완전히 처리했음 */
out:
    return ret;
}
```

### 2.6 mmio-mod.c: pre/post 콜백에서의 접근 정보 기록

#### pre() - MMIO 접근 전 정보 기록

```c
/* arch/x86/mm/mmio-mod.c:128 */
static void pre(struct kmmio_probe *p, struct pt_regs *regs,
                unsigned long addr)
{
    struct trap_reason *my_reason = &get_cpu_var(pf_reason);
    struct mmiotrace_rw *my_trace = &get_cpu_var(cpu_trace);
    const unsigned long instptr = instruction_pointer(regs);

    /* fault를 발생시킨 명령어를 디코딩하여 접근 유형 판별 */
    const enum reason_type type = get_ins_type(instptr);
    struct remap_trace *trace = p->private;

    /* 한 CPU에서 동시에 두 개의 trace가 활성화되면 BUG */
    if (my_reason->active_traces)
        die_kmmio_nesting_error(regs, addr);
    else
        my_reason->active_traces++;

    my_reason->type = type;
    my_reason->addr = addr;
    my_reason->ip = instptr;

    /* 물리 주소 계산: 가상 주소 - probe 시작 + 물리 시작 */
    my_trace->phys = addr - trace->probe.addr + trace->phys;
    my_trace->map_id = trace->id;

    /* PC(Program Counter) 기록 옵션 */
    if (trace_pc)
        my_trace->pc = instptr;
    else
        my_trace->pc = 0;

    switch (type) {
    case REG_READ:
        my_trace->opcode = MMIO_READ;
        my_trace->width = get_ins_mem_width(instptr);
        /* 읽기 값은 아직 모름 → post()에서 기록 */
        break;
    case REG_WRITE:
        my_trace->opcode = MMIO_WRITE;
        my_trace->width = get_ins_mem_width(instptr);
        /* 쓰기 값은 레지스터에서 바로 추출 가능 */
        my_trace->value = get_ins_reg_val(instptr, regs);
        break;
    case IMM_WRITE:
        my_trace->opcode = MMIO_WRITE;
        my_trace->width = get_ins_mem_width(instptr);
        /* immediate 값은 명령어에서 직접 추출 */
        my_trace->value = get_ins_imm_val(instptr);
        break;
    default:
        my_trace->opcode = MMIO_UNKNOWN_OP;
        my_trace->width = 0;
        /* 알 수 없는 명령어: opcode 3바이트 기록 */
        my_trace->value = (*ip) << 16 | *(ip + 1) << 8 | *(ip + 2);
    }
    put_cpu_var(cpu_trace);
    put_cpu_var(pf_reason);
}
```

#### post() - MMIO 접근 후 결과 기록

```c
/* arch/x86/mm/mmio-mod.c:193 */
static void post(struct kmmio_probe *p, unsigned long condition,
                 struct pt_regs *regs)
{
    struct trap_reason *my_reason = &get_cpu_var(pf_reason);
    struct mmiotrace_rw *my_trace = &get_cpu_var(cpu_trace);

    my_reason->active_traces--;
    if (my_reason->active_traces) {
        pr_emerg("unexpected post handler");
        BUG();
    }

    switch (my_reason->type) {
    case REG_READ:
        /*
         * ★ 핵심: single-step 후 레지스터에 MMIO 읽기 결과가 들어있다.
         * 예: mov eax, [mmio_addr]  →  실행 후 eax에 값이 있음
         * my_reason->ip를 통해 어떤 레지스터인지 파악하고 값 추출.
         */
        my_trace->value = get_ins_reg_val(my_reason->ip, regs);
        break;
    default:
        /* 쓰기는 pre()에서 이미 기록함 */
        break;
    }

    /* ★ trace 시스템에 이벤트 기록 ★ */
    mmio_trace_rw(my_trace);
    put_cpu_var(cpu_trace);
    put_cpu_var(pf_reason);
}
```

### 2.7 프로브 등록/해제

#### register_kmmio_probe() - 프로브 등록

```c
/* arch/x86/mm/kmmio.c:436 */
int register_kmmio_probe(struct kmmio_probe *p)
{
    unsigned long flags;
    int ret = 0;
    unsigned long size = 0;
    /* 페이지 경계로 정렬 */
    unsigned long addr = p->addr & PAGE_MASK;
    const unsigned long size_lim = p->len + (p->addr & ~PAGE_MASK);
    unsigned int l;
    pte_t *pte;

    local_irq_save(flags);
    arch_spin_lock(&kmmio_lock);

    /* 동일 주소에 이미 프로브가 있으면 -EEXIST */
    if (get_kmmio_probe(addr)) {
        ret = -EEXIST;
        goto out;
    }

    pte = lookup_address(addr, &l);
    if (!pte) {
        ret = -EINVAL;
        goto out;
    }

    kmmio_count++;
    list_add_rcu(&p->list, &kmmio_probes);

    /* 프로브가 커버하는 모든 페이지를 무장 */
    while (size < size_lim) {
        if (add_kmmio_fault_page(addr + size))
            pr_err("Unable to set page fault.\n");
        size += page_level_size(l);  /* 4KB 또는 2MB씩 증가 */
    }
out:
    arch_spin_unlock(&kmmio_lock);
    local_irq_restore(flags);
    return ret;
}
```

#### unregister_kmmio_probe() - 프로브 해제 (RCU 3단계)

```c
/* arch/x86/mm/kmmio.c:536 */
void unregister_kmmio_probe(struct kmmio_probe *p)
{
    /*
     * 해제는 3단계로 진행된다:
     *
     * 1단계: release_kmmio_fault_page()
     *    - disarm (PTE 복원)
     *    - 아직 in-flight인 fault 핸들러가 있을 수 있으므로
     *      kmmio_page_table에서는 아직 제거하지 않음
     *
     * 2단계: remove_kmmio_fault_pages() (RCU 콜백)
     *    - RCU grace period 후 kmmio_page_table에서 제거
     *    - 이 시점에서 모든 in-flight fault 핸들러가 완료됨이 보장됨
     *
     * 3단계: rcu_free_kmmio_fault_pages() (또 다른 RCU 콜백)
     *    - kmmio_fault_page 구조체를 실제로 kfree()
     */

    /* ... 코드 생략: release_kmmio_fault_page로 각 페이지 해제 ... */

    drelease->release_list = release_list;
    call_rcu(&drelease->rcu, remove_kmmio_fault_pages);
}
```

### 2.8 mmiotrace 사용법

```bash
# 1. debugfs 마운트 (이미 마운트되어 있을 수 있음)
mount -t debugfs none /sys/kernel/debug

# 2. mmiotrace 활성화
echo mmiotrace > /sys/kernel/debug/tracing/current_tracer

# 3. 실시간으로 trace 출력 확인
cat /sys/kernel/debug/tracing/trace_pipe &

# 4. 추적할 드라이버 로드 또는 I/O 수행
modprobe <driver>
# 또는 이미 로드된 드라이버의 I/O 활동

# 5. 추적 중지
echo nop > /sys/kernel/debug/tracing/current_tracer
```

### 2.9 mmiotrace 출력 형식 분석

mmiotrace는 4가지 이벤트 타입을 출력한다:

```
MAP 이벤트 (ioremap 호출):
  mmiotrace: MAP 12345 0xfe000000 0xfe001000 0xffffc90000100000 0x0

  의미: MAP <map_id> <phys_start> <phys_end> <virt_addr> <offset>
  - ioremap()이 호출되어 물리 주소가 가상 주소에 매핑됨
  - 이 시점부터 해당 영역의 MMIO 접근이 추적됨

WRITE 이벤트 (MMIO 쓰기):
  mmiotrace: W 4 0xfe000010 0x00000001 0xffffffff81234567 12345 0

  의미: W <width> <phys_addr> <value> <PC> <timestamp> <map_id>
  - CPU가 MMIO 주소에 값을 쓴 것을 포착
  - width: 1, 2, 4, 8 바이트
  - PC: 쓰기를 수행한 코드의 주소

READ 이벤트 (MMIO 읽기):
  mmiotrace: R 4 0xfe000014 0x0000cafe 0xffffffff81234589 12346 0

  의미: R <width> <phys_addr> <value> <PC> <timestamp> <map_id>
  - CPU가 MMIO 주소에서 값을 읽은 것을 포착
  - value: 디바이스가 반환한 값 (single-step 후 레지스터에서 추출)

UNMAP 이벤트 (iounmap 호출):
  mmiotrace: UNMAP 12345 0x0 0xffffc90000100000 0x0

  의미: UNMAP <map_id> <unused> <virt_addr> <unused>
  - iounmap()이 호출되어 매핑이 해제됨
```

### 2.10 mmiotrace의 SMP 처리

mmiotrace는 SMP 환경에서 정확한 추적을 보장하기 어렵다. 이 문제를 해결하기 위해 `enter_uniprocessor()`를 통해 다른 CPU를 오프라인으로 만든다:

```c
/* arch/x86/mm/mmio-mod.c:368 */
static void enter_uniprocessor(void)
{
    int cpu;
    int err;

    /* 부트 CPU를 제외한 모든 CPU를 다운 */
    cpus_read_lock();
    cpumask_copy(downed_cpus, cpu_online_mask);
    cpumask_clear_cpu(cpumask_first(cpu_online_mask), downed_cpus);
    if (num_online_cpus() > 1)
        pr_notice("Disabling non-boot CPUs...\n");
    cpus_read_unlock();

    for_each_cpu(cpu, downed_cpus) {
        err = remove_cpu(cpu);
        if (!err)
            pr_info("CPU%d is down.\n", cpu);
        else
            pr_err("Error taking CPU%d down: %d\n", cpu, err);
    }

    if (num_online_cpus() > 1)
        pr_warn("multiple CPUs still online, may miss events.\n");
}
```

### 2.11 mmiotrace 한계점 정리

| 한계 | 원인 | 영향 |
|------|------|------|
| 매우 느림 | 매 MMIO 접근마다 #PF + single-step + #DB 발생 | 디바이스 초기화가 수십 배 느려짐 |
| SMP 이벤트 누락 | PTE 복원~재무장 사이 다른 CPU 접근 통과 | 멀티코어에서 이벤트 놓침 |
| CPU 오프라인 부작용 | enter_uniprocessor()로 다른 CPU 다운 | 시스템 성능 크게 저하 |
| 명령어 디코딩 한계 | x86 명령어 세트의 복잡성 | 일부 명령어를 UNKNOWN_OP로 처리 |
| ioremap만 추적 | 직접 물리 주소 접근은 감지 못함 | ioremap 사용하지 않는 접근 놓침 |


## 3. IOMMU 기반 DMA Tracing 구현 방법

IOMMU 기반 DMA tracing은 CPU MMU tracing과 대칭적 구조를 가지지만, 디바이스 측 특성 때문에 여러 변형이 존재한다.

### 3.1 방법 A: Non-Recoverable Fault (간단하지만 DMA 실패)

#### 원리

```
┌─────────────────────────────────────────────────────┐
│  방법 A: Non-Recoverable Fault                      │
│                                                     │
│  ① iommu_unmap()으로 특정 IOVA 매핑 제거            │
│                                                     │
│  ② 디바이스가 DMA 시도                              │
│     → IOMMU가 변환 실패 감지                        │
│     → DMAR Fault Status Register에 기록             │
│     → MSI 인터럽트 발생                             │
│                                                     │
│  ③ dmar_fault() IRQ 핸들러 호출                     │
│     → Fault Recording Register에서 정보 추출:       │
│       - type: Read/Write                            │
│       - source_id: 디바이스 BDF                     │
│       - addr: fault 발생 IOVA                       │
│       - fault_reason: 실패 원인 코드                │
│     → dmar_fault_do_one()으로 로그 출력             │
│                                                     │
│  ④ DMA 실패! 디바이스는 에러 상태로 진입            │
│     → NVMe의 경우: 타임아웃 → 컨트롤러 리셋        │
│     → 복구 불가능 → 한 번만 관찰 가능               │
└─────────────────────────────────────────────────────┘
```

#### 구현 코드

```c
/* Non-Recoverable DMA Fault 유발 방법 */

#include <linux/iommu.h>
#include <linux/pci.h>

/*
 * Step 1: 디바이스의 IOMMU domain 획득
 */
struct pci_dev *pdev = ... ;  /* NVMe PCI 디바이스 */
struct iommu_domain *domain = iommu_get_domain_for_dev(&pdev->dev);
if (!domain) {
    pr_err("IOMMU domain not found (IOMMU 비활성?)\n");
    return -ENODEV;
}

/*
 * Step 2: 추적할 IOVA의 매핑 제거
 * 예: NVMe CQ의 IOVA 주소를 알고 있다고 가정
 */
unsigned long target_iova = 0x...; /* CQ의 IOVA 주소 */
size_t size = PAGE_SIZE;

/* iommu_unmap()은 IOMMU 페이지 테이블에서 해당 IOVA 엔트리를 제거한다 */
size_t unmapped = iommu_unmap(domain, target_iova, size);
if (unmapped != size)
    pr_warn("unmapped %zu bytes (expected %zu)\n", unmapped, size);

/*
 * Step 3: 이제 디바이스가 해당 IOVA로 DMA하면 dmar_fault()가 호출됨
 * 아래의 dmar_fault() 코드가 자동으로 실행된다 (IRQ 핸들러로 등록되어 있으므로)
 */
```

#### Intel VT-d dmar_fault() 코드 상세 분석

```c
/* drivers/iommu/intel/dmar.c:1936 */
irqreturn_t dmar_fault(int irq, void *dev_id)
{
    struct intel_iommu *iommu = dev_id;
    int reg, fault_index;
    u32 fault_status;
    unsigned long flag;
    static DEFINE_RATELIMIT_STATE(rs,
                                  DEFAULT_RATELIMIT_INTERVAL,
                                  DEFAULT_RATELIMIT_BURST);

    raw_spin_lock_irqsave(&iommu->register_lock, flag);

    /* ★ Fault Status Register (FSTS) 읽기 ★
     * 이 레지스터는 fault 발생 여부, overflow, 진행 중인 fault 등을 나타냄
     *
     * Bit 0 (PFO): Primary Fault Overflow
     * Bit 1 (PPF): Primary Pending Fault (처리 대기 중인 fault 있음)
     * Bit 6 (FRI): Fault Record Index (어느 레코드부터 읽을지)
     */
    fault_status = readl(iommu->reg + DMAR_FSTS_REG);
    if (fault_status && __ratelimit(&rs))
        pr_err("DRHD: handling fault status reg %x\n", fault_status);

    /* PPF(Primary Pending Fault) 비트가 없으면 처리할 fault 없음 */
    if (!(fault_status & DMA_FSTS_PPF))
        goto unlock_exit;

    /* Fault Record Index: 어디서부터 읽을지 */
    fault_index = dma_fsts_fault_record_index(fault_status);
    /* Fault Record Register의 오프셋 */
    reg = cap_fault_reg_offset(iommu->cap);

    while (1) {
        bool ratelimited = !__ratelimit(&rs);
        u8 fault_reason;
        u16 source_id;
        u64 guest_addr;
        u32 pasid;
        int type;
        u32 data;
        bool pasid_present;

        /* ★ Fault Recording Register 읽기 (128비트 = 16바이트) ★
         *
         * 각 Fault Record는 16바이트이며 다음 정보를 포함:
         *
         *  [127:64] - 상위 64비트:
         *    - F (bit 127): Fault 유효 비트
         *    - T (bit 126): Type (0=Write, 1=Read)
         *    - FR (bits 103:96): Fault Reason
         *    - PASID (bits 95:76): Process Address Space ID
         *
         *  [95:64] - 중간:
         *    - SID (bits 79:64): Source ID (PCI BDF)
         *
         *  [63:0] - 하위 64비트:
         *    - FI (bits 63:12): Fault Info (Faulting IOVA의 상위 비트)
         */
        data = readl(iommu->reg + reg +
                     fault_index * PRIMARY_FAULT_REG_LEN + 12);

        /* F(Fault) 비트 체크: 0이면 더 이상 fault 없음 */
        if (!(data & DMA_FRCD_F))
            break;

        if (!ratelimited) {
            /* fault 원인 코드 추출 */
            fault_reason = dma_frcd_fault_reason(data);
            /* DMA 유형: 0=Write, 1=Read */
            type = dma_frcd_type(data);
            /* PASID */
            pasid = dma_frcd_pasid_value(data);

            /* Source ID(BDF) 읽기 */
            data = readl(iommu->reg + reg +
                         fault_index * PRIMARY_FAULT_REG_LEN + 8);
            source_id = dma_frcd_source_id(data);
            pasid_present = dma_frcd_pasid_present(data);

            /* Faulting 주소 읽기 (하위 64비트) */
            guest_addr = dmar_readq(iommu->reg + reg +
                                    fault_index * PRIMARY_FAULT_REG_LEN);
            guest_addr = dma_frcd_page_addr(guest_addr);
        }

        /* ★ Fault Record의 F 비트를 클리어하여 "처리 완료" 표시 ★ */
        writel(DMA_FRCD_F, iommu->reg + reg +
               fault_index * PRIMARY_FAULT_REG_LEN + 12);

        raw_spin_unlock_irqrestore(&iommu->register_lock, flag);

        if (!ratelimited)
            /* ★ fault 정보 로깅 ★ */
            dmar_fault_do_one(iommu, type, fault_reason,
                              pasid_present ? pasid : IOMMU_PASID_INVALID,
                              source_id, guest_addr);

        fault_index++;
        if (fault_index >= cap_num_fault_regs(iommu->cap))
            fault_index = 0;  /* 순환 버퍼 */
        raw_spin_lock_irqsave(&iommu->register_lock, flag);
    }

    /* Fault Status 클리어: PFO, PPF, PRO 비트 모두 해제 */
    writel(DMA_FSTS_PFO | DMA_FSTS_PPF | DMA_FSTS_PRO,
           iommu->reg + DMAR_FSTS_REG);

unlock_exit:
    raw_spin_unlock_irqrestore(&iommu->register_lock, flag);
    return IRQ_HANDLED;
}
```

#### dmar_fault_do_one() - 실제 로그 출력

```c
/* drivers/iommu/intel/dmar.c:1899 */
static int dmar_fault_do_one(struct intel_iommu *iommu, int type,
                             u8 fault_reason, u32 pasid, u16 source_id,
                             unsigned long long addr)
{
    const char *reason;
    int fault_type;

    /* fault_reason 코드를 사람이 읽을 수 있는 문자열로 변환 */
    reason = dmar_get_fault_reason(fault_reason, &fault_type);

    /* 출력 예시:
     * [DMA Read NO_PASID] Request device [00:04.0] fault addr 0x100000
     *   [fault reason 0x06] PTE Read access is not set
     *
     * source_id >> 8 = bus number
     * PCI_SLOT(source_id & 0xFF) = device number
     * PCI_FUNC(source_id & 0xFF) = function number
     */
    if (pasid == IOMMU_PASID_INVALID)
        pr_err("[%s NO_PASID] Request device [%02x:%02x.%d] "
               "fault addr 0x%llx [fault reason 0x%02x] %s\n",
               type ? "DMA Read" : "DMA Write",
               source_id >> 8, PCI_SLOT(source_id & 0xFF),
               PCI_FUNC(source_id & 0xFF), addr,
               fault_reason, reason);

    /* IOMMU 페이지 테이블 덤프 (디버깅용) */
    dmar_fault_dump_ptes(iommu, source_id, addr, pasid);

    return 0;
}
```

#### Fault Reason 코드 의미

```c
/* drivers/iommu/intel/dmar.c:1739 */
static const char *dma_remap_fault_reasons[] = {
    "Software",                                    /* 0x00 */
    "Present bit in root entry is clear",          /* 0x01 */
    "Present bit in context entry is clear",       /* 0x02 */
    "Invalid context entry",                       /* 0x03 */
    "Access beyond MGAW",                          /* 0x04 */
    "PTE Write access is not set",                 /* 0x05 */
    "PTE Read access is not set",                  /* 0x06 ← unmap 시 이것 */
    "Next page table ptr is invalid",              /* 0x07 */
    "Root table address invalid",                  /* 0x08 */
    "Context table ptr is invalid",                /* 0x09 */
    "non-zero reserved fields in RTP",             /* 0x0A */
    "non-zero reserved fields in CTP",             /* 0x0B */
    "non-zero reserved fields in PTE",             /* 0x0C */
    "PCE for translation request specifies blocking", /* 0x0D */
};
```

iommu_unmap() 후 DMA 접근 시 주로 fault_reason 0x06 또는 0x07이 발생한다.

#### 한계와 문제점

- DMA가 실패하므로 NVMe 타임아웃 → 컨트롤러 리셋 발생
- 한 번의 fault만 관찰 가능 (이후 디바이스는 에러 상태)
- 프로덕션 환경에서 절대 사용 불가
- mmiotrace의 단계 ③④(복구+재무장)가 불가능


### 3.2 방법 B: PRI (Page Request Interface) 기반 Recoverable Fault

이 방법은 mmiotrace와 가장 유사한 대칭 구조를 가진다. PRI를 지원하는 디바이스만 사용 가능하다.

#### 원리: mmiotrace와의 대칭

```
┌────────────────────────────────┬─────────────────────────────────┐
│     mmiotrace (CPU MMU)        │      PRI 기반 DMA trace         │
│                                │      (IOMMU)                    │
├────────────────────────────────┼─────────────────────────────────┤
│ ① arm_kmmio_fault_page()      │ ① iommu_unmap(domain, iova)     │
│    PTE에서 Present bit 제거    │    IOMMU PTE에서 매핑 제거       │
│                                │                                 │
│ ② CPU가 MMIO 접근 시도        │ ② 디바이스가 DMA 시도            │
│    → Page Fault (#PF)          │    → IOMMU Translation Fault     │
│    → kmmio_handler() 호출      │    → PRQ에 Page Request 기록     │
│                                │    → prq_event_thread() 호출     │
│                                │                                 │
│ ③ pre_handler()에서 기록       │ ③ intel_prq_report()에서 기록    │
│    접근 유형, 주소, 값          │    → iommu_report_device_fault() │
│                                │    접근 유형, 주소, PASID         │
│                                │                                 │
│ ④ disarm + single-step        │ ④ iommu_map() + page_response   │
│    PTE 복원 → 명령어 1개 실행  │    매핑 복구 → DMA 재시도 허용   │
│    → #DB에서 re-arm             │    → 응답 후 re-unmap            │
└────────────────────────────────┴─────────────────────────────────┘
```

#### PRI 동작 흐름 상세

```
디바이스가 DMA 시도 (예: GPU의 page table walk)
  │
  ▼ IOMMU: 해당 IOVA에 매핑 없음!
  │
  │ ★ Non-recoverable fault와의 차이점 ★
  │ PRI 지원 디바이스는 바로 실패하지 않고 "Page Request"를 보냄.
  │ 디바이스는 DMA를 멈추고 응답을 기다림.
  │
  ▼ IOMMU 하드웨어: Page Request Queue(PRQ)에 엔트리 기록
  │
  │  ┌─────────────────────────────────────────────┐
  │  │  PRQ Entry (struct page_req_dsc, 32바이트)   │
  │  │                                             │
  │  │  type:8          - 엔트리 타입              │
  │  │  pasid_present:1 - PASID 유효 여부          │
  │  │  rid:16          - Requester ID (PCI BDF)   │
  │  │  pasid:20        - Process Address Space ID │
  │  │  exe_req:1       - 실행 권한 요청           │
  │  │  pm_req:1        - 특권 모드 요청           │
  │  │  rd_req:1        - 읽기 요청                │
  │  │  wr_req:1        - 쓰기 요청                │
  │  │  lpig:1          - Last Page In Group       │
  │  │  prg_index:9     - Page Request Group Index │
  │  │  addr:52         - 요청 주소 (4KB 정렬)     │
  │  └─────────────────────────────────────────────┘
  │
  ▼ IOMMU → MSI 인터럽트 발생
  │
  ▼ prq_event_thread(irq, iommu)
  │  │
  │  ├─ PRQ head/tail 확인
  │  ├─ 각 PRQ 엔트리 순회
  │  ├─ 유효성 검사 (canonical address, privilege mode 등)
  │  ├─ device_rbtree_find()로 디바이스 검색
  │  │
  │  └─ intel_prq_report(iommu, dev, req)
  │       │
  │       ▼
  │     iommu_report_device_fault(dev, &event)
  │       │
  │       ├─ find_fault_handler(): 적절한 핸들러 찾기
  │       ├─ iopf_group_alloc(): fault 그룹 할당
  │       └─ domain->iopf_handler(group): 도메인 핸들러 호출
  │
  ▼ 도메인 핸들러에서:
  │  ├─ 접근 정보 기록 (주소, R/W, PASID)
  │  ├─ 필요한 매핑 수행: iommu_map(domain, iova, paddr, size, prot)
  │  └─ iopf_group_response(group, IOMMU_PAGE_RESP_SUCCESS)
  │       │
  │       ▼
  │     intel_iommu_page_response()
  │       │
  │       ▼
  │     qi_submit_sync(): Queued Invalidation으로 Page Group Response 전송
  │       │
  │       ▼
  │     IOMMU → 디바이스: "다시 시도해도 됨"
  │
  ▼ 디바이스 DMA 재시도 → 성공!
  │
  ▼ (선택) 다시 매핑 제거 → 다음 DMA도 추적
```

#### 커널 코드 분석: prq.c

##### page_req_dsc 구조체

```c
/* drivers/iommu/intel/prq.c:17 */
struct page_req_dsc {
    union {
        struct {
            u64 type:8;            /* 엔트리 타입 */
            u64 pasid_present:1;   /* PASID 필드가 유효한지 */
            u64 rsvd:7;            /* 예약 */
            u64 rid:16;            /* Requester ID = PCI BDF
                                    * Bus[15:8] Dev[7:3] Func[2:0] */
            u64 pasid:20;          /* Process Address Space ID
                                    * SVA에서 프로세스 구분에 사용 */
            u64 exe_req:1;         /* 실행 권한 요청 */
            u64 pm_req:1;          /* Privilege Mode 요청 */
            u64 rsvd2:10;
        };
        u64 qw_0;  /* Quadword 0 */
    };
    union {
        struct {
            u64 rd_req:1;          /* 읽기 요청 */
            u64 wr_req:1;          /* 쓰기 요청 */
            u64 lpig:1;            /* Last Page In Group
                                    * 1이면 이 그룹의 마지막 페이지
                                    * 응답은 이 엔트리에만 보내면 됨 */
            u64 prg_index:9;       /* Page Request Group Index
                                    * 같은 그룹의 요청들을 묶는 ID */
            u64 addr:52;           /* Faulting 주소의 상위 52비트
                                    * 실제 주소 = addr << 12 (4KB 정렬) */
        };
        u64 qw_1;  /* Quadword 1 */
    };
    u64 qw_2;     /* 미래 확장용 */
    u64 qw_3;     /* 미래 확장용 */
};
```

##### prq_event_thread() - PRQ 인터럽트 핸들러

```c
/* drivers/iommu/intel/prq.c:197 */
static irqreturn_t prq_event_thread(int irq, void *d)
{
    struct intel_iommu *iommu = d;
    struct page_req_dsc *req;
    int head, tail, handled;
    struct device *dev;
    u64 address;

    /* PPR(Pending Page Request) 비트 클리어 → 새 인터럽트 수신 가능 */
    writel(DMA_PRS_PPR, iommu->reg + DMAR_PRS_REG);

    /* PRQ의 head/tail 레지스터 읽기 */
    tail = dmar_readq(iommu->reg + DMAR_PQT_REG) & PRQ_RING_MASK;
    head = dmar_readq(iommu->reg + DMAR_PQH_REG) & PRQ_RING_MASK;
    handled = (head != tail);

    while (head != tail) {
        req = &iommu->prq[head / sizeof(*req)];
        address = (u64)req->addr << VTD_PAGE_SHIFT;  /* addr 필드를 실제 주소로 */

        /* 유효성 검사 1: canonical address 여부 */
        if (unlikely(!is_canonical_address(address))) {
            pr_err("IOMMU: %s: Address is not canonical\n", iommu->name);
bad_req:
            handle_bad_prq_event(iommu, req, QI_RESP_INVALID);
            goto prq_advance;
        }

        /* 유효성 검사 2: privilege mode에서 R/W 동시 요청은 불가 */
        if (unlikely(req->pm_req && (req->rd_req | req->wr_req))) {
            pr_err("IOMMU: %s: Page request in Privilege Mode\n",
                   iommu->name);
            goto bad_req;
        }

        /* 유효성 검사 3: execute + read 동시 요청은 미지원 */
        if (unlikely(req->exe_req && req->rd_req)) {
            pr_err("IOMMU: %s: Execution request not supported\n",
                   iommu->name);
            goto bad_req;
        }

        /* Stop Marker: lpig=1이지만 rd/wr 모두 0이면 stop marker → 무시 */
        if (unlikely(req->lpig && !req->rd_req && !req->wr_req))
            goto prq_advance;

        /* ★ 디바이스 찾기 및 fault 보고 ★ */
        mutex_lock(&iommu->iopf_lock);
        dev = device_rbtree_find(iommu, req->rid);
        if (!dev) {
            mutex_unlock(&iommu->iopf_lock);
            goto bad_req;
        }

        /* ★ IOPF 프레임워크를 통해 fault 보고 ★ */
        intel_prq_report(iommu, dev, req);
        mutex_unlock(&iommu->iopf_lock);

prq_advance:
        head = (head + sizeof(*req)) & PRQ_RING_MASK;
    }

    /* head 포인터 업데이트 → IOMMU에 "여기까지 처리했다" 알림 */
    dmar_writeq(iommu->reg + DMAR_PQH_REG, tail);

    /* PRQ overflow 처리 */
    if (readl(iommu->reg + DMAR_PRS_REG) & DMA_PRS_PRO) {
        pr_info_ratelimited("IOMMU: %s: PRQ overflow detected\n",
                            iommu->name);
        head = dmar_readq(iommu->reg + DMAR_PQH_REG) & PRQ_RING_MASK;
        tail = dmar_readq(iommu->reg + DMAR_PQT_REG) & PRQ_RING_MASK;
        if (head == tail) {
            iopf_queue_discard_partial(iommu->iopf_queue);
            writel(DMA_PRS_PRO, iommu->reg + DMAR_PRS_REG);
        }
    }

    if (!completion_done(&iommu->prq_complete))
        complete(&iommu->prq_complete);

    return IRQ_RETVAL(handled);
}
```

##### intel_prq_report() - PRQ 엔트리를 IOPF 이벤트로 변환

```c
/* drivers/iommu/intel/prq.c:175 */
static void intel_prq_report(struct intel_iommu *iommu, struct device *dev,
                             struct page_req_dsc *desc)
{
    struct iopf_fault event = { };

    /* IOPF 이벤트 구성 */
    event.fault.type = IOMMU_FAULT_PAGE_REQ;

    /* 주소: PRQ의 addr 필드는 상위 52비트, 4KB 정렬이므로 << 12 */
    event.fault.prm.addr = (u64)desc->addr << VTD_PAGE_SHIFT;

    /* PASID */
    event.fault.prm.pasid = desc->pasid;

    /* Group ID: 같은 그룹의 요청을 묶는 인덱스 */
    event.fault.prm.grpid = desc->prg_index;

    /* 접근 권한: rd_req, wr_req, exe_req, pm_req를 IOPF 플래그로 변환 */
    event.fault.prm.perm = prq_to_iommu_prot(desc);

    /* 플래그 설정 */
    if (desc->lpig)
        event.fault.prm.flags |= IOMMU_FAULT_PAGE_REQUEST_LAST_PAGE;
    if (desc->pasid_present) {
        event.fault.prm.flags |= IOMMU_FAULT_PAGE_REQUEST_PASID_VALID;
        event.fault.prm.flags |= IOMMU_FAULT_PAGE_RESPONSE_NEEDS_PASID;
    }

    /* ★ 범용 IOPF 프레임워크에 전달 ★ */
    iommu_report_device_fault(dev, &event);
}
```

#### 커널 코드 분석: io-pgfault.c (IOPF 프레임워크)

##### iommu_report_device_fault() - 중앙 fault 라우팅

```c
/* drivers/iommu/io-pgfault.c:214 */
int iommu_report_device_fault(struct device *dev, struct iopf_fault *evt)
{
    struct iommu_attach_handle *attach_handle;
    struct iommu_fault *fault = &evt->fault;
    struct iommu_fault_param *iopf_param;
    struct iopf_group abort_group = {};
    struct iopf_group *group;

    /* Step 1: 이 디바이스/PASID에 대한 fault handler 찾기 */
    attach_handle = find_fault_handler(dev, evt);
    if (!attach_handle)
        goto err_bad_iopf;

    /* Step 2: 디바이스의 fault parameter 가져오기 (참조 카운트 증가) */
    iopf_param = iopf_get_dev_fault_param(dev);
    if (WARN_ON(!iopf_param))
        goto err_bad_iopf;

    /* Step 3: 마지막 페이지가 아닌 부분 fault는 일단 큐에 저장 */
    if (!(fault->prm.flags & IOMMU_FAULT_PAGE_REQUEST_LAST_PAGE)) {
        int ret = report_partial_fault(iopf_param, fault);
        iopf_put_dev_fault_param(iopf_param);
        return ret;
    }

    /* Step 4: 마지막 페이지 fault → 그룹 할당 및 핸들러 호출 */
    group = iopf_group_alloc(iopf_param, evt, &abort_group);
    if (group == &abort_group)
        goto err_abort;

    group->attach_handle = attach_handle;

    /* ★ 도메인에 등록된 iopf_handler 호출 ★
     * SVA의 경우: iommu_sva_handle_iopf() → 프로세스 페이지 테이블 접근 → 매핑
     * 커스텀 핸들러: 사용자 정의 동작 수행
     */
    if (group->attach_handle->domain->iopf_handler(group))
        goto err_abort;

    return 0;

err_abort:
    dev_warn_ratelimited(dev, "iopf with pasid %d aborted\n",
                         fault->prm.pasid);
    iopf_group_response(group, IOMMU_PAGE_RESP_FAILURE);
    /* ... cleanup ... */
    return 0;

err_bad_iopf:
    if (fault->type == IOMMU_FAULT_PAGE_REQ)
        iopf_error_response(dev, evt);
    return -EINVAL;
}
```

##### iopf_group_response() - Page Response 전송

```c
/* drivers/iommu/io-pgfault.c:322 */
void iopf_group_response(struct iopf_group *group,
                         enum iommu_page_response_code status)
{
    struct iommu_fault_param *fault_param = group->fault_param;
    struct iopf_fault *iopf = &group->last_fault;
    struct device *dev = group->fault_param->dev;
    const struct iommu_ops *ops = dev_iommu_ops(dev);

    /* Page Response 구성 */
    struct iommu_page_response resp = {
        .pasid = iopf->fault.prm.pasid,
        .grpid = iopf->fault.prm.grpid,
        .code = status,
        /* status 값:
         * IOMMU_PAGE_RESP_SUCCESS  = 매핑 완료, 재시도 OK
         * IOMMU_PAGE_RESP_FAILURE  = 실패, 이후 fault 중단
         * IOMMU_PAGE_RESP_INVALID  = 처리 불가, 재시도 하지 마라
         */
    };

    mutex_lock(&fault_param->lock);
    if (!list_empty(&group->pending_node)) {
        /* ★ IOMMU 드라이버의 page_response 콜백 호출 ★
         * Intel의 경우: intel_iommu_page_response()
         */
        ops->page_response(dev, &group->last_fault, &resp);
        list_del_init(&group->pending_node);
    }
    mutex_unlock(&fault_param->lock);
}
```

##### intel_iommu_page_response() - QI를 통한 Page Group Response

```c
/* drivers/iommu/intel/prq.c:372 */
void intel_iommu_page_response(struct device *dev, struct iopf_fault *evt,
                               struct iommu_page_response *msg)
{
    struct device_domain_info *info = dev_iommu_priv_get(dev);
    struct intel_iommu *iommu = info->iommu;
    u8 bus = info->bus, devfn = info->devfn;
    struct iommu_fault_page_request *prm;
    struct qi_desc desc;
    bool pasid_present;
    u16 sid;

    prm = &evt->fault.prm;
    sid = PCI_DEVID(bus, devfn);
    pasid_present = prm->flags & IOMMU_FAULT_PAGE_REQUEST_PASID_VALID;

    /* Queued Invalidation을 통해 Page Group Response 전송
     *
     * QI descriptor 구성:
     * - QI_PGRP_PASID: 대상 PASID
     * - QI_PGRP_DID: 디바이스 SID (BDF)
     * - QI_PGRP_RESP_CODE: 응답 코드 (SUCCESS/FAILURE/INVALID)
     * - QI_PGRP_IDX: Page Request Group Index (어떤 요청 그룹에 대한 응답인지)
     */
    desc.qw0 = QI_PGRP_PASID(prm->pasid) | QI_PGRP_DID(sid) |
               QI_PGRP_PASID_P(pasid_present) |
               QI_PGRP_RESP_CODE(msg->code) |
               QI_PGRP_RESP_TYPE;
    desc.qw1 = QI_PGRP_IDX(prm->grpid);
    desc.qw2 = 0;
    desc.qw3 = 0;

    /* ★ IOMMU의 QI에 descriptor를 제출하고 완료 대기 ★
     * IOMMU가 이 descriptor를 처리하면 디바이스에 Page Response를 보낸다.
     * 디바이스는 이 응답을 받으면 멈춰있던 DMA를 재시도한다.
     */
    qi_submit_sync(iommu, &desc, 1, 0);
}
```

#### PRI 지원 요구사항 및 현실

| 요구사항 | 설명 |
|----------|------|
| PCIe PRI Capability | 디바이스가 PCIe Extended Capability에서 PRI를 광고해야 함 |
| IOMMU PRQ 지원 | Intel VT-d의 경우 Page Request Queue(PRQ) 기능 필요 |
| ATS 지원 | PRI는 ATS(Address Translation Services)를 전제로 함 |
| 커널 드라이버 지원 | 드라이버가 PRI를 활성화하고 IOPF 핸들러를 등록해야 함 |

**NVMe 디바이스가 PRI를 지원하는가?**

대부분의 NVMe 디바이스는 PRI를 지원하지 않는다. PRI는 주로 SVA(Shared Virtual Addressing) 용도로 설계되었으며, 다음 디바이스들이 주로 지원한다:

- Intel GPU (Gen12+)
- AMD GPU (RDNA2+)
- 일부 고급 네트워크 카드 (Intel DSA, IAX)
- 일부 FPGA 가속기

NVMe는 전통적으로 호스트가 할당한 고정 DMA 버퍼를 사용하므로 PRI가 필요 없다.


### 3.3 방법 C: IOMMU Dirty Tracking + 폴링

DMA를 방해하지 않으면서 "어떤 페이지가 DMA write를 받았는지"를 추적하는 방법이다.

#### 원리

```
┌───────────────────────────────────────────────────────────────┐
│  방법 C: IOMMU Dirty Tracking                                │
│                                                               │
│  ★ 핵심 차이: fault를 유발하지 않음!                          │
│                                                               │
│  IOMMU 하드웨어가 DMA write된 페이지를 자동으로 표시          │
│  → 주기적으로 dirty bitmap을 폴링하여 변경 감지               │
│                                                               │
│  ┌──────────────┐                                             │
│  │ IOMMU PTE    │ Access bit (A) + Dirty bit (D)              │
│  │              │ CPU PTE와 동일한 개념!                       │
│  │ [D][A][...]  │                                             │
│  └──────────────┘                                             │
│        │                                                      │
│        ▼                                                      │
│  디바이스 DMA write → IOMMU가 D bit 설정                      │
│        │                                                      │
│        ▼                                                      │
│  커널이 주기적으로 dirty bitmap 수집                           │
│  (IOMMU_HWPT_GET_DIRTY_BITMAP ioctl)                          │
│        │                                                      │
│        ▼                                                      │
│  dirty 페이지 목록으로 "어떤 영역에 DMA write가 있었는지" 파악│
│        │                                                      │
│        ▼                                                      │
│  D bit 클리어 → 다음 폴링 주기에 새 변경 감지                 │
└───────────────────────────────────────────────────────────────┘
```

#### Intel VT-d Dirty Tracking 커널 코드

```c
/* drivers/iommu/intel/iommu.c:49 */
static int intel_iommu_set_dirty_tracking(struct iommu_domain *domain,
                                          bool enable);

/* drivers/iommu/intel/iommu.c:2900 */
static const struct iommu_dirty_ops intel_second_stage_dirty_ops = {
    IOMMU_PT_DIRTY_OPS(vtdss),
    .set_dirty_tracking = intel_iommu_set_dirty_tracking,
};
```

Dirty tracking을 활성화하면 IOMMU 하드웨어가 second-level 페이지 테이블 엔트리의 Dirty bit를 자동으로 설정한다.

#### iommufd를 통한 사용 방법

```c
/* 사용자 공간에서 iommufd를 통한 dirty tracking */

#include <linux/iommufd.h>

int fd = open("/dev/iommu", O_RDWR);

/* 1. Dirty tracking 활성화 */
struct iommu_hwpt_set_dirty_tracking dirty_cfg = {
    .size = sizeof(dirty_cfg),
    .hwpt_id = hwpt_id,        /* 하드웨어 페이지 테이블 ID */
    .flags = IOMMU_HWPT_DIRTY_TRACKING_ENABLE,
};
ioctl(fd, IOMMU_HWPT_SET_DIRTY_TRACKING, &dirty_cfg);

/* 2. Dirty bitmap 수집 */
struct iommu_hwpt_get_dirty_bitmap dirty_bitmap = {
    .size = sizeof(dirty_bitmap),
    .hwpt_id = hwpt_id,
    .iova = start_iova,
    .length = region_size,
    .page_size = PAGE_SIZE,
    .data = (__u64)bitmap_buffer,  /* 결과를 받을 비트맵 버퍼 */
    .flags = IOMMU_HWPT_GET_DIRTY_BITMAP_NO_CLEAR,  /* 읽기만, D bit 유지 */
};
ioctl(fd, IOMMU_HWPT_GET_DIRTY_BITMAP, &dirty_bitmap);

/* 3. bitmap_buffer에서 dirty 페이지 확인 */
for (i = 0; i < bitmap_size; i++) {
    if (test_bit(i, bitmap_buffer)) {
        printf("Page at IOVA 0x%lx was written by DMA\n",
               start_iova + i * PAGE_SIZE);
    }
}
```

#### 특성 비교

| 특성 | Fault 기반 | Dirty Tracking |
|------|-----------|----------------|
| DMA 영향 | 중단/지연 | 영향 없음 |
| 정밀도 | 매 접근 단위 | 폴링 주기 단위 |
| 정보량 | R/W, 주소, BDF, 타이밍 | Write된 페이지 집합만 |
| 하드웨어 요구 | PRI (fault 복구 시) | VT-d SSAD (Second Stage A/D) |
| 주 용도 | 학습/디버깅 | VM 라이브 마이그레이션 |


### 3.4 방법 D: Custom Kernel Module로 구현하기

학습 목적으로 Non-Recoverable Fault 방식의 DMA trace 모듈 골격을 제시한다.

```c
/*
 * dma_fault_tracer.c - IOMMU DMA Fault Tracing 학습용 커널 모듈
 *
 * WARNING: 이 모듈은 학습/이해 목적이다.
 * DMA가 실패하므로 디바이스가 에러 상태로 진입한다.
 * 프로덕션 환경에서 사용하면 안 된다.
 */

#include <linux/module.h>
#include <linux/iommu.h>
#include <linux/pci.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DMA Fault Tracing Module (Educational)");

static char *pci_addr = "0000:00:04.0";
module_param(pci_addr, charp, 0444);
MODULE_PARM_DESC(pci_addr, "PCI device address (BDF) to trace");

static unsigned long target_iova = 0;
module_param(target_iova, ulong, 0444);
MODULE_PARM_DESC(target_iova, "Target IOVA address to unmap");

static struct pci_dev *pdev;
static struct iommu_domain *domain;
static phys_addr_t saved_paddr;  /* 복구를 위해 원래 물리 주소 저장 */

/*
 * 모듈 초기화: 디바이스의 IOMMU 매핑을 제거하여 fault 유도
 */
static int __init dma_tracer_init(void)
{
    struct device *dev;
    phys_addr_t paddr;

    /* Step 1: PCI 디바이스 찾기 */
    pdev = pci_get_domain_bus_and_slot(0,  /* domain */
                                       0,  /* bus - 실제 값으로 변경 필요 */
                                       PCI_DEVFN(4, 0));  /* 예시 */
    if (!pdev) {
        pr_err("dma_tracer: PCI device %s not found\n", pci_addr);
        return -ENODEV;
    }
    dev = &pdev->dev;

    /* Step 2: IOMMU domain 가져오기 */
    domain = iommu_get_domain_for_dev(dev);
    if (!domain) {
        pr_err("dma_tracer: No IOMMU domain (is IOMMU enabled?)\n");
        pr_err("dma_tracer: Boot with intel_iommu=on\n");
        pci_dev_put(pdev);
        return -ENODEV;
    }

    pr_info("dma_tracer: Domain type: %d\n", domain->type);

    if (domain->type == IOMMU_DOMAIN_IDENTITY) {
        pr_err("dma_tracer: Device uses identity mapping.\n");
        pr_err("dma_tracer: Need iommu.strict=1 and DMA domain.\n");
        pci_dev_put(pdev);
        return -EINVAL;
    }

    if (!target_iova) {
        pr_err("dma_tracer: target_iova parameter required\n");
        pci_dev_put(pdev);
        return -EINVAL;
    }

    /* Step 3: 현재 매핑 정보 저장 (나중에 복구용) */
    saved_paddr = iommu_iova_to_phys(domain, target_iova);
    if (!saved_paddr) {
        pr_err("dma_tracer: No mapping at IOVA 0x%lx\n", target_iova);
        pci_dev_put(pdev);
        return -EINVAL;
    }

    pr_info("dma_tracer: IOVA 0x%lx -> PA 0x%llx\n",
            target_iova, (unsigned long long)saved_paddr);
    pr_info("dma_tracer: Unmapping to trigger DMA fault...\n");

    /* Step 4: ★ 매핑 제거 → 디바이스의 DMA가 fault 발생 ★ */
    iommu_unmap(domain, target_iova, PAGE_SIZE);

    pr_info("dma_tracer: IOVA 0x%lx unmapped. DMA fault will occur.\n");
    pr_info("dma_tracer: Watch dmesg for DMAR fault messages.\n");
    pr_info("dma_tracer: Expected: [DMA Read/Write] fault addr 0x%lx\n",
            target_iova);

    /*
     * 이 시점에서 디바이스가 target_iova로 DMA하면:
     *
     * 1. IOMMU가 변환 실패 감지
     * 2. Fault Recording Register에 기록
     * 3. MSI 인터럽트 → dmar_fault() 호출
     * 4. dmar_fault_do_one()이 dmesg에 로그 출력:
     *    [DMA Read NO_PASID] Request device [00:04.0]
     *    fault addr 0x<target_iova>
     *    [fault reason 0x06] PTE Read access is not set
     */

    return 0;
}

/*
 * 모듈 정리: 매핑 복구
 */
static void __exit dma_tracer_exit(void)
{
    int ret;

    if (domain && saved_paddr && target_iova) {
        pr_info("dma_tracer: Restoring mapping: IOVA 0x%lx -> PA 0x%llx\n",
                target_iova, (unsigned long long)saved_paddr);

        /* 매핑 복구 */
        ret = iommu_map(domain, target_iova, saved_paddr,
                        PAGE_SIZE, IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
        if (ret)
            pr_err("dma_tracer: Failed to restore mapping: %d\n", ret);
        else
            pr_info("dma_tracer: Mapping restored successfully.\n");
    }

    if (pdev)
        pci_dev_put(pdev);

    pr_info("dma_tracer: Module unloaded.\n");
}

module_init(dma_tracer_init);
module_exit(dma_tracer_exit);
```

#### Makefile

```makefile
obj-m += dma_fault_tracer.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
```

#### 사용 방법

```bash
# 1. IOMMU 활성화 부팅
# 커널 파라미터: intel_iommu=on iommu.strict=1

# 2. NVMe의 IOVA 주소 확인 (예: iommu_debug)
# CQ나 데이터 버퍼의 IOVA를 사전에 파악해야 함

# 3. 모듈 로드
insmod dma_fault_tracer.ko target_iova=0x100000

# 4. dmesg에서 fault 메시지 확인
dmesg | grep -i "dmar\|fault\|dma_tracer"

# 5. 모듈 언로드 (매핑 복구)
rmmod dma_fault_tracer
```


## 4. 두 메커니즘의 대칭 비교표

```
┌────────────┬─────────────────────────────┬──────────────────────────────┐
│   특성      │  mmiotrace (CPU MMU)         │  IOMMU DMA trace             │
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 방향       │ CPU → Device                │ Device → Host                │
│            │ (CPU가 MMIO 공간에 접근)     │ (디바이스가 호스트 메모리 DMA)│
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 페이지     │ CPU 페이지 테이블            │ IOMMU 페이지 테이블          │
│ 테이블     │ (x86 4-level/5-level)       │ (VT-d second-level 또는      │
│            │                             │  first-level)                │
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 무장 방법  │ clear_pte_presence()        │ iommu_unmap()                │
│            │ PTE에서 Present bit = 0     │ IOMMU PTE 엔트리 제거        │
│            │ + flush_tlb_one_kernel()    │ + IOTLB flush                │
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ Fault 유형 │ #PF (Page Fault, INT 14)    │ DMAR Fault (Non-recoverable) │
│            │                             │ 또는 PRQ (Page Request Queue)│
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 핸들러     │ kmmio_handler()             │ dmar_fault() (non-recoverable│
│            │ (do_page_fault에서 호출)     │ iopf_handler() (PRI 기반)    │
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 정보 추출  │ 명령어 디코딩:              │ Fault Record:                │
│            │ - get_ins_type(): R/W 판별  │ - type: R/W                  │
│            │ - get_ins_mem_width(): 크기 │ - source_id: BDF             │
│            │ - get_ins_reg_val(): 값     │ - addr: fault IOVA           │
│            │ - instruction_pointer: PC   │ - fault_reason: 원인 코드    │
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 복구 방법  │ PTE 복원                    │ iommu_map()                  │
│            │ (disarm_kmmio_fault_page)   │ (IOMMU PTE에 매핑 재생성)    │
│            │ + X86_EFLAGS_TF (single     │ + iopf_group_response()      │
│            │   step)                     │   (Page Response 전송)       │
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 재무장     │ post_kmmio_handler()에서    │ Page Response 전송 후        │
│            │ arm_kmmio_fault_page()      │ 다시 iommu_unmap()           │
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 성능 영향  │ 매우 느림                   │ 느림                         │
│            │ (매 접근마다 #PF + #DB)     │ (매 DMA마다 fault + response)│
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 멀티 CPU   │ 다른 CPU의 접근 놓칠 수 있음│ IOMMU가 중앙에서 처리하므로  │
│            │ (PTE 복원 ~ 재무장 사이)    │ 모든 DMA 접근 포착 가능      │
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 복구 가능  │ ◯ (항상 복구 가능)          │ PRI 있으면 ◯                │
│ 여부       │ single-step이 보장          │ PRI 없으면 ✕ (DMA 실패)     │
├────────────┼─────────────────────────────┼──────────────────────────────┤
│ 실제 사용  │ GPU 드라이버 리버스 엔지니어링│ VM 라이브 마이그레이션       │
│ 사례       │ (Nouveau 프로젝트)          │ (dirty tracking)             │
│            │ 디바이스 초기화 시퀀스 파악  │ VFIO passthrough             │
└────────────┴─────────────────────────────┴──────────────────────────────┘
```


## 5. 실전 구현 가이드: NVMe DMA 추적 모듈

### 5.1 목표

NVMe 디바이스가 호스트 메모리에 DMA할 때 (CQ write, data transfer) 추적한다.

```
NVMe 디바이스의 DMA 대상:
┌────────────────────────────────────────────┐
│ 1. Completion Queue (CQ) 쓰기             │
│    - NVMe 컨트롤러 → 호스트 메모리        │
│    - 4KB 단위 페이지에 CQE(16B) 기록      │
│                                            │
│ 2. 데이터 전송 (read command 결과)         │
│    - NVMe 컨트롤러 → 호스트 메모리        │
│    - PRP/SGL이 가리키는 데이터 버퍼에 기록 │
│                                            │
│ 3. Submission Queue (SQ) 읽기             │
│    - NVMe 컨트롤러 ← 호스트 메모리        │
│    - SQE(64B)를 읽어감                     │
│                                            │
│ 4. 데이터 전송 (write command 데이터)      │
│    - NVMe 컨트롤러 ← 호스트 메모리        │
│    - PRP/SGL이 가리키는 데이터를 읽어감    │
└────────────────────────────────────────────┘
```

### 5.2 전제 조건

```bash
# 커널 파라미터 (GRUB 설정)
GRUB_CMDLINE_LINUX="intel_iommu=on iommu.strict=1"

# 확인
dmesg | grep -i iommu
# 출력 예:
# DMAR: IOMMU enabled
# DMAR: Intel(R) Virtualization Technology for Directed I/O

# NVMe 디바이스의 IOMMU 그룹 확인
ls -la /sys/bus/pci/devices/0000:00:04.0/iommu_group/
```

### 5.3 구현 전략

```
┌─────────────────────────────────────────────────────────────┐
│ NVMe DMA 추적 전략                                          │
│                                                             │
│ 단계 1: NVMe 디바이스의 iommu_domain 획득                   │
│   struct iommu_domain *domain =                             │
│       iommu_get_domain_for_dev(&nvme_pdev->dev);            │
│                                                             │
│ 단계 2: CQ의 IOVA 주소 파악                                 │
│   - NVMe 드라이버가 dma_alloc_coherent()로 CQ 할당          │
│   - /sys/kernel/debug/iommu/ 또는 소스 코드에서 추적         │
│   - 또는 ftrace로 dma_alloc_coherent 반환값 캡처            │
│                                                             │
│ 단계 3: 해당 IOVA unmap                                     │
│   iommu_unmap(domain, cq_iova, PAGE_SIZE);                  │
│                                                             │
│ 단계 4: DMAR fault 관찰                                     │
│   dmesg에 나타나는 fault 메시지 확인                         │
│   [DMA Write NO_PASID] Request device [00:04.0]             │
│   fault addr 0x<cq_iova>                                    │
│   [fault reason 0x05] PTE Write access is not set           │
│                                                             │
│ 단계 5: 매핑 복구 (필수!)                                   │
│   iommu_map(domain, cq_iova, cq_paddr,                     │
│             PAGE_SIZE, IOMMU_READ|IOMMU_WRITE, GFP_KERNEL); │
│   → NVMe 컨트롤러가 다시 CQ에 기록할 수 있음               │
│     그러나 이미 타임아웃이 발생했을 수 있다                  │
└─────────────────────────────────────────────────────────────┘
```

### 5.4 주의사항 (실패 가능성)

```
┌─────────────────────────────────────────────────────────────┐
│ ★ 중요: 이 방법의 한계 ★                                    │
│                                                             │
│ 1. NVMe는 PRI를 지원하지 않음                               │
│    → recoverable fault 불가능                               │
│    → DMA가 실패함 → NVMe 타임아웃                           │
│                                                             │
│ 2. NVMe 타임아웃 시퀀스:                                     │
│    DMA fault → CQ에 completion 못 씀                        │
│    → nvme_timeout() (30초 후)                               │
│    → nvme_dev_disable() → 컨트롤러 리셋                     │
│    → 모든 I/O가 중단됨                                      │
│                                                             │
│ 3. 매핑을 빨리 복구해도:                                    │
│    - NVMe 컨트롤러가 이미 DMA를 포기했을 수 있음            │
│    - completion이 유실되어 I/O가 완료되지 않을 수 있음       │
│                                                             │
│ 4. 따라서:                                                  │
│    - 1회성 관찰만 가능                                      │
│    - mmiotrace처럼 연속적 추적은 불가능                     │
│    - 학습/이해 목적으로만 가치가 있음                        │
└─────────────────────────────────────────────────────────────┘
```

### 5.5 대안: QEMU에서 IOMMU Tracing 에뮬레이션

실제 하드웨어 없이 안전하게 실험하려면 QEMU를 사용한다.

```bash
# QEMU에서 Intel IOMMU를 활성화하여 NVMe 에뮬레이션
qemu-system-x86_64 \
    -machine q35 \
    -device intel-iommu,intremap=on,device-iotlb=on \
    -device nvme,drive=nvme0,serial=deadbeef \
    -drive file=nvme.img,format=raw,if=none,id=nvme0 \
    ...
```

QEMU의 Intel IOMMU 에뮬레이션 소스(`hw/i386/intel_iommu.c`)에 tracing 코드를 추가하면 Guest의 모든 DMA 변환을 안전하게 관찰할 수 있다.

```
QEMU 기반 접근의 장점:
┌──────────────────────────────────────────────┐
│ ✓ DMA fault가 발생해도 하드웨어 손상 없음    │
│ ✓ IOMMU 페이지 테이블을 자유롭게 조작 가능   │
│ ✓ IOMMU translation을 한 단계씩 추적 가능    │
│ ✓ Guest 커널의 dmar_fault()도 정상 동작      │
│ ✓ PRI 에뮬레이션도 이론적으로 가능           │
│ ✓ 반복 실험이 용이 (스냅샷/복원)             │
└──────────────────────────────────────────────┘
```


## 6. IOPF 프레임워크 상세 구조

커널의 IOPF(IO Page Fault) 프레임워크는 IOMMU 벤더 독립적인 page fault 처리 인프라를 제공한다. 이것은 mmiotrace의 kmmio 프레임워크에 대응하는 IOMMU 측 인프라이다.

```
┌─────────────────────────────────────────────────────────────────────┐
│                     IOPF 프레임워크 구조                             │
│                                                                     │
│  IOMMU 하드웨어 (Intel VT-d)                                        │
│  ┌──────────────────────────┐                                       │
│  │ Page Request Queue (PRQ) │                                       │
│  │ [entry0][entry1][...]    │                                       │
│  └───────────┬──────────────┘                                       │
│              │ IRQ                                                   │
│              ▼                                                       │
│  ┌──────────────────────────┐                                       │
│  │ prq_event_thread()       │  ← Intel 드라이버 (벤더 특화)          │
│  │   intel_prq_report()     │                                       │
│  └───────────┬──────────────┘                                       │
│              │                                                       │
│              ▼ iommu_report_device_fault(dev, &event)                │
│  ┌──────────────────────────┐                                       │
│  │ IOPF Framework           │  ← 벤더 독립 계층 (io-pgfault.c)      │
│  │                          │                                       │
│  │ find_fault_handler()     │  fault에 맞는 핸들러 검색              │
│  │ iopf_group_alloc()       │  fault 그룹 할당                      │
│  │ domain->iopf_handler()   │  도메인 핸들러 호출                   │
│  │ iopf_group_response()    │  Page Response 전송                   │
│  └───────────┬──────────────┘                                       │
│              │                                                       │
│              ▼ ops->page_response(dev, evt, &resp)                   │
│  ┌──────────────────────────┐                                       │
│  │ intel_iommu_page_response│  ← Intel 드라이버 (벤더 특화)          │
│  │   qi_submit_sync()       │  QI를 통해 Page Response 전송          │
│  └──────────────────────────┘                                       │
│                                                                     │
│  ★ kmmio 프레임워크와의 대응:                                        │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │ kmmio_probes (프로브 리스트)   ↔  iopf_queue (fault queue)   │    │
│  │ kmmio_fault_page (무장 페이지) ↔  iommu_fault_param (fault 정보)│  │
│  │ pre_handler + post_handler     ↔  iopf_handler + page_response│  │
│  │ register_kmmio_probe()         ↔  iopf_queue_add_device()    │    │
│  │ unregister_kmmio_probe()       ↔  iopf_queue_remove_device() │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
```

### IOPF 핵심 데이터 구조

```c
/* include/linux/iommu.h */

/* Page Request 이벤트 데이터 */
struct iommu_fault_page_request {
    u32 flags;           /* IOMMU_FAULT_PAGE_REQUEST_* 플래그 */
    u32 pasid;           /* Process Address Space ID */
    u32 grpid;           /* Page Request Group Index */
    u32 perm;            /* 요청된 권한 (IOMMU_FAULT_PERM_*) */
    u64 addr;            /* Faulting 주소 */
    u64 private_data[2]; /* 디바이스 specific 데이터 */
};

/* Generic fault 데이터 */
struct iommu_fault {
    u32 type;   /* IOMMU_FAULT_PAGE_REQ */
    struct iommu_fault_page_request prm;
};

/* Fault + 리스트 노드 */
struct iopf_fault {
    struct iommu_fault fault;
    struct list_head list;        /* pending list에 연결 */
};

/* Fault 그룹 (같은 grpid를 가진 fault들의 묶음) */
struct iopf_group {
    struct iopf_fault last_fault;           /* 그룹의 마지막 fault */
    struct list_head faults;                /* 그룹에 속한 fault 리스트 */
    size_t fault_count;                     /* fault 개수 */
    struct list_head pending_node;          /* pending 리스트 노드 */
    struct work_struct work;                /* 비동기 처리 workqueue */
    struct iommu_attach_handle *attach_handle; /* 도메인 연결 핸들 */
    struct iommu_fault_param *fault_param;  /* 디바이스 fault 파라미터 */
    struct list_head node;                  /* 핸들러 자체 리스트용 */
    u32 cookie;
};

/* Per-device fault 파라미터 */
struct iommu_fault_param {
    struct mutex lock;
    refcount_t users;              /* 참조 카운트 */
    struct rcu_head rcu;
    struct device *dev;
    struct iopf_queue *queue;      /* 이 디바이스가 속한 IOPF 큐 */
    struct list_head queue_list;   /* 큐의 디바이스 리스트 노드 */
    struct list_head partial;      /* 아직 그룹이 완성되지 않은 fault */
    struct list_head faults;       /* 응답 대기 중인 fault 그룹 */
};

/* Page Response */
enum iommu_page_response_code {
    IOMMU_PAGE_RESP_SUCCESS = 0,  /* 성공: 매핑 완료, 재시도 가능 */
    IOMMU_PAGE_RESP_INVALID,      /* 무효: 재시도하지 마라 */
    IOMMU_PAGE_RESP_FAILURE,      /* 실패: 이후 모든 fault 중단 */
};

struct iommu_page_response {
    u32 pasid;
    u32 grpid;
    u32 code;   /* enum iommu_page_response_code */
};
```


## 7. 미래: ATS/PRI/SVA가 보편화되면

### 현재 상황과 미래 전망

```
┌─────────────────────────────────────────────────────────────────┐
│                    현재 → 미래 진화 경로                         │
│                                                                 │
│  ┌──────────────────┐                                           │
│  │ ATS              │  Address Translation Services              │
│  │ (현재: GPU, DSA) │  디바이스가 IOMMU에 주소 변환을 요청       │
│  │                  │  → IOTLB에 캐시하여 직접 변환 가능         │
│  │                  │  → 변환 요청을 가로채면 DMA 전에 추적 가능 │
│  └────────┬─────────┘                                           │
│           │ 전제조건                                             │
│           ▼                                                      │
│  ┌──────────────────┐                                           │
│  │ PRI              │  Page Request Interface                    │
│  │ (현재: GPU, DSA) │  IOMMU fault 시 디바이스가 "Page Request"  │
│  │                  │  → OS가 매핑 후 "Page Response"            │
│  │                  │  → mmiotrace와 동일한 패턴 구현 가능!      │
│  │                  │  → recoverable fault 기반 DMA tracing      │
│  └────────┬─────────┘                                           │
│           │ 활용                                                 │
│           ▼                                                      │
│  ┌──────────────────┐                                           │
│  │ SVA              │  Shared Virtual Addressing                 │
│  │ (진행 중)        │  프로세스 주소 공간을 디바이스와 공유       │
│  │                  │  → CPU page fault와 IOMMU fault가 통합     │
│  │                  │  → 디바이스가 사용자 공간 포인터 직접 사용  │
│  │                  │  → 기존 mmiotrace 인프라 확장으로           │
│  │                  │    통합 tracing 가능                        │
│  └──────────────────┘                                           │
│                                                                 │
│  미래 시나리오:                                                  │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ NVMe가 PRI를 지원하는 세상이 오면:                       │    │
│  │                                                         │    │
│  │ 1. NVMe 컨트롤러가 CQ에 DMA write 시도                  │    │
│  │ 2. IOMMU: 매핑 없음 → PRQ에 Page Request 기록           │    │
│  │ 3. 커널 핸들러: 접근 정보 기록 (주소, R/W, 타이밍)      │    │
│  │ 4. iommu_map()으로 매핑 복구                             │    │
│  │ 5. Page Response: "다시 시도해도 됨"                     │    │
│  │ 6. NVMe가 DMA 재시도 → 성공!                            │    │
│  │ 7. 다시 매핑 제거 → 다음 DMA도 추적                     │    │
│  │                                                         │    │
│  │ → mmiotrace의 완벽한 대칭 구현 달성!                     │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

### CXL과 IOMMU의 융합

CXL(Compute Express Link) 디바이스는 호스트 메모리와 디바이스 메모리를 통합하는 새로운 인터커넥트이다. CXL 2.0+ 디바이스에서는 IOMMU 기반 fault tracking이 더욱 중요해진다:

```
CXL 시대의 메모리 접근 추적:

   CPU ─────────────┐
                    │
   CXL Device ──────┤──── 통합 메모리 공간
                    │     (Host + Device memory)
   GPU ─────────────┘
         │
         ▼
   IOMMU가 모든 접근을 중재
   → Page Fault/Request를 통한
     통합 추적이 자연스러운 모델
```


## 8. 요약: 핵심 인사이트

```
┌─────────────────────────────────────────────────────────────────┐
│                      핵심 인사이트                               │
│                                                                 │
│  1. mmiotrace와 IOMMU DMA tracing은 동일한 설계 패턴이다.       │
│     "매핑 제거 → fault → 기록 → 복구 → 재무장"                 │
│                                                                 │
│  2. 차이점은 "누가 접근하는가"와                                │
│     "어떤 페이지 테이블을 조작하는가"뿐이다.                     │
│                                                                 │
│  3. mmiotrace는 CPU의 single-step (#DB)으로 복구/재무장하지만,  │
│     IOMMU tracing은 PRI의 Page Response로 복구/재무장한다.       │
│                                                                 │
│  4. 현재 NVMe는 PRI를 지원하지 않으므로                         │
│     recoverable fault 기반 추적은 불가능하다.                    │
│     그러나 non-recoverable fault로 1회성 관찰은 가능하다.        │
│                                                                 │
│  5. IOMMU dirty tracking은 fault 없이 DMA write를               │
│     추적할 수 있는 대안이다.                                     │
│                                                                 │
│  6. 이 대칭 구조를 이해하면 커널의 메모리 보호 메커니즘의        │
│     본질을 꿰뚫어 볼 수 있다:                                    │
│     "접근을 차단 → 가로채서 관찰 → 허용 → 다시 차단"            │
│     이것은 OS의 가장 기본적인 제어 패턴이다.                     │
└─────────────────────────────────────────────────────────────────┘
```

### 관련 소스 파일 위치

| 파일 | 역할 |
|------|------|
| `arch/x86/mm/kmmio.c` | mmiotrace 핵심: arm/disarm/fault handler |
| `arch/x86/mm/mmio-mod.c` | mmiotrace 모듈: pre/post callback, enable/disable |
| `drivers/iommu/intel/dmar.c` | Intel DMAR fault handler (non-recoverable) |
| `drivers/iommu/intel/prq.c` | Intel PRQ 처리 + Page Response (recoverable) |
| `drivers/iommu/io-pgfault.c` | 범용 IOPF 프레임워크 |
| `drivers/iommu/iommu.c` | iommu_map/unmap, domain ops |
| `drivers/iommu/intel/iommu.c` | Intel IOMMU domain ops, dirty tracking |
| `include/linux/iommu.h` | iommu_fault, iopf_group, page_response 구조체 |
