#include "nerd.h"
#include "nd_lock.h"
#include "nd_klibc.h"

#define U64_MAX ((nd_uint64_t)(~0ULL))

#define timer_container_of(n) (container_of((n), struct nd_timer, node))

static struct rb_root rbtree_root = RB_ROOT;

nd_uint64_t nd_hw_get_current(void)
{
#if ND_CFG_TICKLESS
    return nd_hw_hrtimer_get_current();
#else
    return nd_hw_tick_get_current();
#endif
}

static nd_timer_t *get_leftmost_timer(void)
{
    struct rb_node *n = rb_first(&rbtree_root);
    return n ? timer_container_of(n) : ND_NULL;
}

static void timer_node_insert(nd_timer_t *timer)
{
    struct rb_node **link = &rbtree_root.rb_node;
    struct rb_node *parent = ND_NULL;

    while (*link) {
        parent = *link;
        if (timer_container_of(parent)->expire_time > timer->expire_time)
            link = &parent->rb_left;
        else
            link = &parent->rb_right;
    }

    rb_link_node(&timer->node, parent, link);
    rb_insert_color(&timer->node, &rbtree_root);
}

static void timer_insert(nd_timer_t *timer)
{
#if ND_CFG_TICKLESS
    nd_uint64_t now = nd_hw_get_current();

    timer->expire_time = (timer->timeout >= U64_MAX/2) ? U64_MAX : (timer->timeout + now);

    timer_node_insert(timer);

    nd_timer_t *leftmost_timer = get_leftmost_timer();
    if (!leftmost_timer)
        return;

    if (&leftmost_timer->node == &timer->node)
        nd_hw_hrtimer_set_expire(leftmost_timer->expire_time);

    if (now >= leftmost_timer->expire_time)
        nd_hw_hrtimer_trigger();
#else
    timer_node_insert(timer);
#endif
}

static void timer_remove(nd_timer_t *timer)
{
    if (RB_EMPTY_NODE(&timer->node))
        return;

#if ND_CFG_TICKLESS
    nd_timer_t *leftmost_timer = get_leftmost_timer();

    if (leftmost_timer && &leftmost_timer->node == &timer->node) {
        rb_erase(&timer->node, &rbtree_root);
        RB_CLEAR_NODE(&timer->node);

        leftmost_timer = get_leftmost_timer();
        if (!leftmost_timer) {
            nd_hw_hrtimer_trigger_clear();
            return;
        }
        nd_hw_hrtimer_set_expire(leftmost_timer->expire_time);
    } else {
        rb_erase(&timer->node, &rbtree_root);
        RB_CLEAR_NODE(&timer->node);
    }

    if (nd_hw_get_current() >= leftmost_timer->expire_time)
        nd_hw_hrtimer_trigger();
#else
    rb_erase(&timer->node, &rbtree_root);
    RB_CLEAR_NODE(&timer->node);
#endif
}

static void timer_expired(void)
{
    for (;;) {
        nd_timer_t *leftmost_timer = get_leftmost_timer();
        if (!leftmost_timer) {
#if ND_CFG_TICKLESS
            nd_hw_hrtimer_trigger_clear();
#endif
            return;
        }

        nd_uint64_t now = nd_hw_get_current();

        if (now < leftmost_timer->expire_time)
            break;

        rb_erase(&leftmost_timer->node, &rbtree_root);
        RB_CLEAR_NODE(&leftmost_timer->node);

        if (leftmost_timer->type == ND_TIMER_TYPE_PERIODIC) {
            leftmost_timer->expire_time += leftmost_timer->timeout;
            timer_node_insert(leftmost_timer);
        }

        if (leftmost_timer->callback)
            leftmost_timer->callback(leftmost_timer->arg);
    }

#if ND_CFG_TICKLESS
    nd_timer_t *leftmost_timer = get_leftmost_timer();
    if (leftmost_timer) {
        nd_hw_hrtimer_set_expire(leftmost_timer->expire_time);
        if (nd_hw_get_current() >= leftmost_timer->expire_time)
            nd_hw_hrtimer_trigger();
    } else {
        nd_hw_hrtimer_trigger_clear();
    }
#endif
}

void nd_timer_process(void)
{
    nd_kernel_def();
    nd_kernel_lock();

    timer_expired();

    nd_kernel_unlock();
}

nd_err_t nd_timer_init(nd_timer_t *timer,
                       const char *name,
                       nd_timer_type_t type,
                       nd_uint64_t timeout,
                       void (*callback)(void *arg),
                       void *arg)
{
    if (!timer || !callback) {
        return ND_EINVAL;
    }

    if (timeout == 0) {
        return ND_EINVAL;
    }

    nd_kernel_def();
    nd_kernel_lock();

    if (name) {
        nd_strncpy(timer->name, name, ND_NAME_MAX_SIZE - 1);
        timer->name[ND_NAME_MAX_SIZE - 1] = '\0';
    } else {
        timer->name[0] = '\0';
    }

    timer->type = type;
    timer->timeout = timeout;
    timer->callback = callback;
    timer->arg = arg;

    RB_CLEAR_NODE(&timer->node);

    nd_kernel_unlock();

    return ND_EOK;
}

nd_err_t nd_timer_start(nd_timer_t *timer)
{
    if (!timer || !timer->callback) {
        return ND_EINVAL;
    }

    nd_kernel_def();
    nd_kernel_lock();

    if (!RB_EMPTY_NODE(&timer->node)) {
        timer_remove(timer);
    }
#if !ND_CFG_TICKLESS
    nd_uint64_t now = nd_hw_get_current();
    timer->expire_time = now + timer->timeout;
#endif

    timer_insert(timer);

    nd_kernel_unlock();

    return ND_EOK;
}

nd_err_t nd_timer_stop(nd_timer_t *timer)
{
    if (!timer) {
        return ND_EINVAL;
    }

    nd_kernel_def();
    nd_kernel_lock();

    timer_remove(timer);

    nd_kernel_unlock();

    return ND_EOK;
}

