#include "nd_ipc.h"

static void nd_ipc_timeout_callback(void *arg)
{
    nd_thread_t *thread = (nd_thread_t *)arg;

    thread->stat = ND_THREAD_STAT_READY;
    thread->error = ND_ETIMEOUT;

    nd_list_remove(&thread->prio_list);
    nd_thread_ready_add_tail(thread);
}

void nd_ipc_suspend(nd_list_t *wait_list, nd_uint64_t timeout)
{
    nd_list_insert_before(wait_list, &nd_current_thread->prio_list);

    nd_current_thread->error = ND_EOK;
    nd_current_thread->stat = ND_THREAD_STAT_BLOCK;

    if (timeout != ND_TIMEOUT_FOREVER) {
        nd_current_thread->timer.timeout = timeout;
        nd_current_thread->timer.callback = nd_ipc_timeout_callback;
        nd_current_thread->timer.arg = nd_current_thread;

        nd_timer_start(&nd_current_thread->timer);
    }
}

nd_thread_t *nd_ipc_resume(nd_list_t *wait_list)
{
    nd_thread_t *thread = nd_list_entry(wait_list->next, nd_thread_t, prio_list);

    nd_timer_stop(&thread->timer);

    thread->error = ND_EOK;
    thread->stat = ND_THREAD_STAT_READY;

    nd_list_remove(&thread->prio_list);
    nd_thread_ready_add_tail(thread);

    return thread;
}
