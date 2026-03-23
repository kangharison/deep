/*
 * list.h - 커널 모듈용 이중 연결 리스트 구현 헤더.
 * 스핀락으로 보호되는 이중 연결 리스트를 제공한다.
 * 컨트롤러 목록(ctrl_list)과 DMA 매핑 목록(host_list, device_list)을 관리하는 데 사용한다.
 * 커널의 기본 list_head 대신 별도 구현을 사용하는 이유: 커스텀 인터페이스와 스핀락 통합.
 */
#ifndef __LIBNVM_HELPER_LIST_H__
#define __LIBNVM_HELPER_LIST_H__

#include <linux/types.h>
#include <linux/spinlock.h>  /* spinlock_t, spin_lock, spin_unlock: 인터럽트 안전한 락 */
#include <linux/compiler.h>  /* __always_inline: 항상 인라인으로 컴파일하도록 강제 */


/* 전방 선언 */
struct list;


/*
 * list_node - 이중 연결 리스트의 노드.
 * 각 관리 대상 구조체(ctrl, map)에 이 노드를 멤버로 포함시키고,
 * container_of 매크로로 노드에서 상위 구조체를 역추적한다.
 */
struct list_node
{
    struct list*        list;   /* 이 노드가 속한 리스트에 대한 참조 (제거 시 필요) */
    struct list_node*   next;   /* 다음 노드를 가리키는 포인터 */
    struct list_node*   prev;   /* 이전 노드를 가리키는 포인터 */
};


/*
 * list - 이중 연결 리스트.
 * head는 더미 노드로 실제 데이터를 담지 않으며, 리스트의 시작과 끝을 연결하는 센티널 역할을 한다.
 * 스핀락으로 동시 접근을 보호한다 (인터럽트 컨텍스트에서도 안전).
 */
struct list
{
    struct list_node    head;   /* 리스트 시작점 (더미 노드, 순환 연결) */
    spinlock_t          lock;   /* 리스트 조작 시 동시성을 보호하는 스핀락 */
};



/*
 * list_node_init - 리스트 노드를 초기화한다.
 * 모든 포인터를 NULL로 설정하여 아직 어떤 리스트에도 속하지 않은 상태로 만든다.
 * __always_inline: 함수 호출 오버헤드 없이 인라인으로 확장된다.
 */
static void __always_inline list_node_init(struct list_node* element)
{
    element->list = NULL;  /* 아직 어떤 리스트에도 속하지 않음 */
    element->next = NULL;
    element->prev = NULL;
}



/*
 * list_next 매크로 - 현재 노드의 다음 노드를 반환한다.
 * 다음 노드가 head(더미 노드)이면 리스트의 끝이므로 NULL을 반환한다.
 * 순환 리스트에서 head를 만나면 한 바퀴 돌아온 것이므로 순회를 종료하는 조건이 된다.
 */
#define list_next(current)  \
    ( ((current)->next != &(current)->list->head) ? (current)->next : NULL )



/*
 * list_init - 리스트를 초기화한다.
 * head 노드가 자기 자신을 가리키도록 설정하여 빈 순환 리스트를 만든다.
 */
void list_init(struct list* list);



/*
 * list_insert - 리스트 끝에 새 노드를 삽입한다.
 * 스핀락으로 보호되어 동시 접근에 안전하다.
 */
void list_insert(struct list* list, struct list_node* element);



/*
 * list_remove - 리스트에서 노드를 제거한다.
 * 스핀락으로 보호되어 동시 접근에 안전하다.
 */
void list_remove(struct list_node* element);



#endif /* __LIBNVM_HELPER_LIST_H__ */
