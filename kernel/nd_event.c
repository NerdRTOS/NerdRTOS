#include "nerd.h"
#include "nd_internal.h"
#include "nd_lock.h"

void nd_event_init(nd_event_t *event)
{
    nd_kernel_def();
    nd_kernel_lock();

    event->set = 0;

    nd_list_init(&event->wait_list);

    nd_kernel_unlock();
}

static nd_bool_t nd_event_is_satisfied(nd_uint32_t set, nd_uint32_t event_set, nd_event_opt_t opt)
{
    if (opt == ND_EVENT_AND && (set & event_set) == event_set)
        return ND_TRUE;
    else if (opt == ND_EVENT_OR && (set & event_set) != 0)
        return ND_TRUE;

    return ND_FALSE;
}

static void nd_event_wakeup(nd_thread_t *thread)
{
    nd_list_remove(&thread->qnode);

    nd_timer_stop(&thread->timer);

    thread->stat = ND_THREAD_STAT_READY;

    nd_thread_ready_add_tail(thread);
}

nd_err_t nd_event_send(nd_event_t *event, nd_uint32_t set)
{
    nd_kernel_def();
    nd_kernel_lock();

    nd_thread_t *thread, *tmp;

    event->set |= set;

    nd_list_for_each_entry_safe(thread, tmp, &event->wait_list, qnode) {
        if (nd_event_is_satisfied(event->set, thread->event_set, thread->event_opt) == ND_TRUE) {
            event->set &= ~thread->event_set;
            nd_event_wakeup(thread);
        }
    }

    nd_scheduler();

    nd_kernel_unlock();

    return ND_EOK;
}

nd_err_t nd_event_recv(nd_event_t *event, nd_uint32_t set, nd_event_opt_t opt, nd_uint64_t timeout)
{
    nd_kernel_def();
    nd_kernel_lock();

    nd_current_thread->event_set = set;
    nd_current_thread->event_opt = opt;

    if (nd_event_is_satisfied(event->set, set, opt) == ND_TRUE) {
        event->set &= ~set;
        nd_kernel_unlock();
        return ND_EOK;
    }

    if (timeout == ND_TIMEOUT_NOWAIT) {
        nd_kernel_unlock();
        return ND_EBUSY;
    }

    nd_thread_pend(&event->wait_list, timeout);

    nd_scheduler();

    nd_kernel_unlock();

    return nd_current_thread->error;
}
