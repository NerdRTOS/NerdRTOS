#include "nerd.h"
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

nd_err_t nd_thread_init(nd_thread_t    *thread,
                        char           *name,
                        void           (*entry)(void *parameter),
                        nd_uint8_t     priority,
                        void           *parameter,
                        void           *stack_addr,
                        nd_uint32_t    stack_size,
                        nd_uint64_t    time_slice)
{
    nd_kernel_def();
    nd_kernel_lock();

    nd_memset(thread, 0, sizeof(nd_thread_t));

    nd_strncpy(thread->name, name, ND_NAME_MAX_SIZE - 1);
    thread->name[ND_NAME_MAX_SIZE - 1] = '\0';

    nd_strncpy(thread->timer.name, name, ND_NAME_MAX_SIZE - 1);
    thread->timer.name[ND_NAME_MAX_SIZE - 1] = '\0';

    nd_strncpy(thread->slice_timer.name, name, ND_NAME_MAX_SIZE - 1);
    thread->slice_timer.name[ND_NAME_MAX_SIZE - 1] = '\0';

    thread->entry = entry;
    thread->parameter = parameter;
    thread->priority = priority;
    thread->init_priority = priority;
    thread->stack_addr = stack_addr;
    thread->stack_size = stack_size;

    nd_list_init(&thread->prio_list);
    nd_list_init(&thread->tlist);
    nd_list_init(&thread->taken_list);

    thread->timer.arg = thread;
    thread->timer.type = ND_TIMER_TYPE_ONE_SHOT;
    thread->timer.timeout = 1;
    thread->timer.callback = ND_NULL;
    RB_CLEAR_NODE(&thread->timer.node);

    thread->time_slice = time_slice;
    thread->slice_left = time_slice;
    thread->slice_start = 0;

    thread->slice_timer.arg = thread;
    thread->slice_timer.type = ND_TIMER_TYPE_ONE_SHOT;
    thread->slice_timer.timeout = time_slice ? time_slice : 1;
    thread->slice_timer.callback = nd_thread_slice_timeout;
    RB_CLEAR_NODE(&thread->slice_timer.node);

    thread->yield = 0;
    thread->stat = ND_THREAD_STAT_INIT;

    nd_memset(thread->stack_addr, 0xAA, thread->stack_size);

    thread->sp = nd_hw_stack_init(thread->entry, thread->parameter,
                                  thread->stack_addr + thread->stack_size, 0);

    nd_list_insert_before(&nd_thread_list, &thread->tlist);

    
    nd_thread_ready_add_tail(thread); 

    nd_kernel_unlock();

    return ND_EOK;
}


nd_err_t nd_thread_suspend(nd_thread_t *thread)
{
    nd_kernel_def();
    nd_kernel_lock();

    /* 如线程持有锁，拒绝挂起，避免优先级反转 */
    if (!nd_list_is_empty(&thread->taken_list)) {
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
        if (nd_list_is_linked(&thread->prio_list)) {
            nd_list_remove(&thread->prio_list);
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

    nd_list_remove(&thread->prio_list);
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

void nd_thread_pend(nd_list_t *wait_list, nd_uint64_t timeout)
{
    nd_list_insert_before(wait_list, &nd_current_thread->prio_list);

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
    nd_thread_t *thread = nd_list_entry(wait_list->next, nd_thread_t, prio_list);

    nd_timer_stop(&thread->timer);

    thread->error = ND_EOK;
    thread->stat = ND_THREAD_STAT_READY;

    nd_list_remove(&thread->prio_list);
    nd_thread_ready_add_tail(thread);

    return thread;
}
