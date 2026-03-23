/*
 * list.c - 커널 모듈용 이중 연결 리스트 구현.
 * 스핀락으로 보호되는 순환 이중 연결 리스트를 제공한다.
 * head는 더미 노드로 데이터를 담지 않으며, 삽입/삭제 시 경계 조건 처리를 단순화한다.
 */
#include "list.h"
#include <linux/types.h>
#include <linux/spinlock.h>  /* spin_lock_init, spin_lock, spin_unlock: 스핀락 API */
#include <linux/printk.h>
#include <asm/errno.h>
#include <linux/compiler.h>  /* likely/unlikely: 분기 예측 힌트 매크로 */



/*
 * list_init - 빈 순환 이중 연결 리스트를 초기화한다.
 * head 노드가 자기 자신의 prev/next를 가리키도록 설정하여 빈 리스트를 표현한다.
 * 스핀락도 초기화하여 이후 동시 접근 보호에 사용한다.
 */
void list_init(struct list* list)
{
    list->head.list = list;         /* head 노드가 속한 리스트를 자기 자신으로 설정한다 */
    list->head.prev = &list->head;  /* head의 이전 = head (빈 리스트에서 자기 자신을 가리킴) */
    list->head.next = &list->head;  /* head의 다음 = head (빈 리스트에서 자기 자신을 가리킴) */

    spin_lock_init(&list->lock);    /* 스핀락을 초기화한다 (최초 1회) */
}



/*
 * list_remove - 리스트에서 노드를 제거한다.
 * 이전 노드의 next와 다음 노드의 prev를 서로 연결하여 현재 노드를 빼낸다.
 * 스핀락으로 보호하여 다른 CPU에서의 동시 조작을 방지한다.
 * likely() 매크로: 조건이 참일 가능성이 높음을 컴파일러에 알려 분기 예측을 최적화한다.
 */
void list_remove(struct list_node* element)
{
    /* element가 유효하고, 리스트에 속해 있으며, head(더미)가 아닌지 확인한다 */
    if (likely(element != NULL && element->list != NULL && element != &element->list->head))
    {
        spin_lock(&element->list->lock);      /* 리스트 스핀락을 획득한다 */
        element->prev->next = element->next;  /* 이전 노드의 next를 다음 노드로 연결한다 */
        element->next->prev = element->prev;  /* 다음 노드의 prev를 이전 노드로 연결한다 */
        spin_unlock(&element->list->lock);    /* 스핀락을 해제한다 */

        element->list = NULL;  /* 리스트 소속을 해제한다 */
        element->next = NULL;  /* 댕글링 포인터를 방지한다 */
        element->prev = NULL;
    }
}



/*
 * list_insert - 리스트 끝(tail)에 새 노드를 삽입한다.
 * 순환 리스트에서 head.prev가 마지막 노드이므로, head.prev 뒤에 삽입하면 tail append가 된다.
 * 스핀락으로 보호하여 동시 삽입/삭제에 안전하다.
 */
void list_insert(struct list* list, struct list_node* element)
{
    struct list_node* last = NULL;

    spin_lock(&list->lock);             /* 리스트 스핀락을 획득한다 */
    last = list->head.prev;             /* 현재 마지막 노드를 가져온다 */
    last->next = element;               /* 기존 마지막 노드의 next를 새 노드로 설정한다 */

    element->list = list;               /* 새 노드가 속한 리스트를 설정한다 */
    element->prev = last;               /* 새 노드의 prev를 기존 마지막 노드로 설정한다 */
    element->next = &list->head;        /* 새 노드의 next를 head로 설정한다 (순환 연결) */

    list->head.prev = element;          /* head의 prev를 새 노드로 갱신한다 (새 노드가 마지막이 됨) */

    spin_unlock(&list->lock);           /* 스핀락을 해제한다 */
}
