#ifndef __ND_LIST_H__
#define __ND_LIST_H__

#include "nd_def.h"

struct nd_list_node
{
    struct nd_list_node *next;
    struct nd_list_node *prev;
};
typedef struct nd_list_node nd_list_t;

static inline void nd_list_init(nd_list_t *list)
{
    list->next = list;
    list->prev = list;
}

static inline void nd_list_insert_after(nd_list_t *node, nd_list_t *new_node)
{
    new_node->next       = node->next;
    new_node->prev       = node;
    node->next->prev     = new_node;
    node->next           = new_node;
}

static inline void nd_list_insert_before(nd_list_t *node, nd_list_t *new_node)
{
    new_node->prev       = node->prev;
    new_node->next       = node;
    node->prev->next     = new_node;
    node->prev           = new_node;
}

static inline void nd_list_remove(nd_list_t *node)
{
    node->prev->next     = node->next;
    node->next->prev     = node->prev;
    node->next           = ND_NULL;
    node->prev           = ND_NULL;
}

static inline nd_bool_t nd_list_is_empty(const nd_list_t *list)
{
    return (nd_bool_t)(list->next == list);
}

static inline nd_bool_t nd_list_is_linked(const nd_list_t *node)
{
    return node->next != ND_NULL;
}

static inline nd_uint32_t nd_list_length(const nd_list_t *list)
{
    nd_uint32_t length = 0;
    const nd_list_t *node;

    for (node = list->next; node != list; node = node->next)
    {
        length++;
    }

    return length;
}

#define nd_list_entry(node, type, member) \
    container_of(node, type, member)

#define nd_list_for_each_entry(pos, head, member) \
    for (pos = nd_list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = nd_list_entry(pos->member.next, typeof(*pos), member))

#define nd_list_for_each_entry_safe(pos, tmp, head, member) \
    for (pos = nd_list_entry((head)->next, typeof(*pos), member), \
         tmp = nd_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = tmp, \
         tmp = nd_list_entry(tmp->member.next, typeof(*tmp), member))

#endif
