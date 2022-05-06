#include <type.h>

struct list_head {
	struct list_head *next, *prev;
};

#define POISON_POINTER_DELTA 0xdead000000000000
#define LIST_POISON1  ((void *) 0x00100100 + POISON_POINTER_DELTA)
#define LIST_POISON2  ((void *) 0x00200200 + POISON_POINTER_DELTA)

#define LIST_HEAD_INIT(name) {&(name), &(name)}

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *new, struct list_head *prev,
		struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}
/* 将new node 加入到head的头部*/
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}
/* 将new node 加入到head的尾部*/
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;

}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

static inline int list_empty(struct list_head *head)
{
	return head->next == head;
}

/* 因为list_head基本上都是嵌入到其他结构体中的，所以我们需要通过链表的指针来获取当前的宿主结构体的指针。 */
/* 	ptr:当前的list_head的指针
 *  type:宿主结构体
 *  member:宿主结构体中的成员（这里因为使用场景的关系，所以这个member基本上都是list_head）
 */
#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/* 遍历list_head */
/* pos:临时变量
 * head:遍历链表
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)


