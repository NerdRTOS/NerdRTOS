#include "nerd.h"
#include "nd_internal.h"
#include "nd_lock.h"
#include "nd_klibc.h"

nd_list_t  nd_thread_list;

void nd_thread_list_init(void)
{
    nd_list_init(&nd_thread_list);
}

nd_list_t *nd_thread_list_get(void)
{
    return &nd_thread_list;
}

void *nd_thread_stack_alloc(nd_size_t size)
{
    return nd_malloc(size);
}

nd_err_t nd_thread_stack_free(void *stack)
{
    if (stack == ND_NULL) {
        return ND_EINVAL;
    }

    nd_free(stack);

    return ND_EOK;
}

nd_err_t nd_thread_resume(nd_thread_t *thread)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (thread->stat != ND_THREAD_STAT_SUSPEND) {
        nd_kernel_unlock();
        return ND_ERROR;
    }

    thread->stat = ND_THREAD_STAT_READY;

    nd_thread_ready_add_tail(thread);

    nd_scheduler();

    nd_kernel_unlock();

    return ND_EOK;
}

static void nd_thread_pend_timeout(void *arg)
{
    nd_thread_t *thread = (nd_thread_t *)arg;

    thread->stat = ND_THREAD_STAT_READY;
    thread->error = ND_ETIMEOUT;

    nd_list_remove(&thread->qnode);
    nd_thread_ready_add_tail(thread);
}

nd_uint32_t nd_thread_stack_used(nd_thread_t *thread)
{
    nd_uint8_t *p = (nd_uint8_t *)thread->stack_addr;
    nd_uint32_t unused = 0;

    while (unused < thread->stack_size && *p == 0xAA) {
        p++;
        unused++;
    }

    return thread->stack_size - unused;
}

static void nd_thread_wait_insert(nd_list_t *wait_list, nd_thread_t *thread)
{
    nd_thread_t *pos;

    nd_list_for_each_entry(pos, wait_list, qnode) {
        if (pos->priority > thread->priority) {
            nd_list_insert_before(&pos->qnode, &thread->qnode);
            return;
        }
    }

    nd_list_insert_before(wait_list, &thread->qnode);
}

void nd_thread_pend(nd_list_t *wait_list, nd_uint64_t timeout)
{
    nd_thread_wait_insert(wait_list, nd_current_thread);

    nd_current_thread->error = ND_EOK;
    nd_current_thread->stat = ND_THREAD_STAT_BLOCK;

    if (timeout != ND_TIMEOUT_FOREVER) {
        nd_current_thread->timer.timeout = timeout;
        nd_current_thread->timer.callback = nd_thread_pend_timeout;
        nd_current_thread->timer.arg = nd_current_thread;

        nd_timer_start(&nd_current_thread->timer);
    }
}

nd_thread_t *nd_thread_wakeup(nd_list_t *wait_list)
{
    nd_thread_t *thread = nd_list_entry(wait_list->next, nd_thread_t, qnode);

    nd_timer_stop(&thread->timer);

    thread->error = ND_EOK;
    thread->stat = ND_THREAD_STAT_READY;

    nd_list_remove(&thread->qnode);
    nd_thread_ready_add_tail(thread);

    return thread;
}

nd_err_t nd_thread_suspend(nd_thread_t *thread)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (!nd_list_is_empty(&thread->mutex.taken_list)) {
        nd_kernel_unlock();
        return ND_EPERM;
    }

    switch (thread->stat) {
    case ND_THREAD_STAT_READY:
        nd_thread_ready_remove(thread);
        thread->stat = ND_THREAD_STAT_SUSPEND;
        break;
    case ND_THREAD_STAT_RUNNING:
        thread->stat = ND_THREAD_STAT_SUSPEND;
        nd_scheduler();
        break;
    case ND_THREAD_STAT_BLOCK:
        thread->stat = ND_THREAD_STAT_SUSPEND;
        if (nd_list_is_linked(&thread->qnode)) {
            nd_list_remove(&thread->qnode);
        }
        nd_timer_stop(&thread->timer);
        nd_scheduler();
        break;
    default:
        nd_kernel_unlock();
        return ND_ERROR;
    }

    nd_kernel_unlock();

    return ND_EOK;
}

void nd_thread_entry(void (*entry)(void *), void *parameter)
{
    entry(parameter);

    nd_thread_abort(nd_current_thread);

    while (1);
}

static inline nd_bool_t nd_is_thread_essential(nd_thread_t *thread)
{
    return (thread->options & ND_THREAD_OPT_ESSENTIAL) != 0;
}

static inline nd_bool_t nd_is_thread_dead(nd_thread_t *thread)
{
    return thread->stat == ND_THREAD_STAT_DEAD;
}

static nd_err_t nd_thread_init(nd_thread_t     *thread,
                               const char      *name,
                               void            (*entry)(void *parameter),
                               nd_uint8_t      priority,
                               void            *parameter,
                               void            *stack,
                               nd_size_t       stack_size,
                               nd_uint64_t     time_slice)
{
    nd_strncpy(thread->name, name, ND_NAME_MAX_SIZE - 1);
    thread->name[ND_NAME_MAX_SIZE - 1] = '\0';

    nd_strncpy(thread->timer.name, name, ND_NAME_MAX_SIZE - 1);
    thread->timer.name[ND_NAME_MAX_SIZE - 1] = '\0';

    nd_strncpy(thread->slice.slice_timer.name, name, ND_NAME_MAX_SIZE - 1);
    thread->slice.slice_timer.name[ND_NAME_MAX_SIZE - 1] = '\0';

    thread->entry = entry;
    thread->parameter = parameter;
    thread->priority = priority;
    thread->init_priority = priority;
    thread->stack_addr = stack;
    thread->stack_size = stack_size;
    thread->timer.arg = thread;
    thread->timer.type = ND_TIMER_TYPE_ONE_SHOT;

    RB_CLEAR_NODE(&thread->timer.node);

    thread->slice.time_slice = time_slice;
    thread->slice.slice_left = time_slice;
    thread->slice.slice_start = 0;
    thread->slice.slice_timer.arg = thread;
    thread->slice.slice_timer.type = ND_TIMER_TYPE_ONE_SHOT;
    thread->slice.slice_timer.callback = nd_thread_slice_timeout;

    RB_CLEAR_NODE(&thread->slice.slice_timer.node);

    thread->yield = 0;
    thread->stat = ND_THREAD_STAT_INIT;
    thread->mutex.pending = ND_NULL;

    nd_memset(&thread->usage, 0, sizeof(thread->usage));
    nd_memset(&thread->event, 0, sizeof(thread->event));

    nd_list_init(&thread->qnode);
    nd_list_init(&thread->tlist);
    nd_list_init(&thread->mutex.taken_list);
    nd_list_init(&thread->join_list);

    nd_memset(thread->stack_addr, 0xAA, thread->stack_size);

    thread->sp = nd_hw_stack_init(thread->entry, thread->parameter,
                                  thread->stack_addr + thread->stack_size, 0);

    return ND_EOK;
}

nd_err_t nd_thread_create(nd_thread_t   *thread,
                          const char    *name,
                          void          (*entry)(void *parameter),
                          nd_uint8_t    priority,
                          void          *parameter,
                          void          *stack,
                          nd_size_t     stack_size,
                          nd_uint32_t   options,
                          nd_uint64_t   time_slice)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (!thread || !name || !entry || !stack || !stack_size || priority >= ND_THREAD_PRIORITY_MAX) {
        nd_kernel_unlock();
        return ND_EINVAL;
    }

    if (nd_thread_init(thread, name, entry, priority,
                       parameter, stack, stack_size, time_slice) != ND_EOK)
    {
        nd_kernel_unlock();
        return ND_ERROR;
    }

    thread->options = options;
    thread->stat = ND_THREAD_STAT_READY;

    nd_list_insert_before(&nd_thread_list, &thread->tlist);
    nd_thread_ready_add_tail(thread);

    nd_scheduler();

    nd_kernel_unlock();

    return ND_EOK;
}

nd_err_t nd_thread_abort(nd_thread_t *thread)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (!thread) {
        nd_kernel_unlock();
        return ND_EINVAL;
    }

    if (nd_is_thread_dead(thread)) {
        nd_kernel_unlock();
        return ND_EOK;
    }

    if (nd_is_thread_essential(thread)) {
        nd_kernel_unlock();
        return ND_EPERM;
    }

    switch (thread->stat) {
    case ND_THREAD_STAT_READY:
        if (nd_list_is_linked(&thread->qnode)) {
            nd_thread_ready_remove(thread);
        }
        break;

    case ND_THREAD_STAT_BLOCK:
        if (nd_list_is_linked(&thread->qnode)) {
            nd_list_remove(&thread->qnode);
        }
        break;

    case ND_THREAD_STAT_RUNNING:
        break;

    case ND_THREAD_STAT_SUSPEND:
        break;

    default:
        break;
    }

    if (nd_list_is_linked(&thread->tlist)) {
        nd_list_remove(&thread->tlist);
    }

    thread->stat = ND_THREAD_STAT_DEAD;

    nd_timer_stop(&thread->timer);
    nd_timer_stop(&thread->slice.slice_timer);

    while (!nd_list_is_empty(&thread->join_list)) {
        nd_thread_wakeup(&thread->join_list);
    }

    if (thread == nd_current_thread) {
        nd_scheduler();
    }
    nd_kernel_unlock();

    return ND_EOK;
}

nd_err_t nd_thread_join(nd_thread_t *thread, nd_uint64_t timeout)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (!thread) {
        nd_kernel_unlock();
        return ND_EINVAL;
    }

    if (thread == nd_current_thread) {
        nd_kernel_unlock();
        return ND_EDEADLK;
    }

    if (thread->stat == ND_THREAD_STAT_DEAD) {
        nd_kernel_unlock();
        return ND_EOK;
    }

    if (timeout == ND_TIMEOUT_NOWAIT) {
        nd_kernel_unlock();
        return ND_EBUSY;
    }

    nd_thread_pend(&thread->join_list, timeout);

    nd_scheduler();

    nd_kernel_unlock();

    return nd_current_thread->error;
}
