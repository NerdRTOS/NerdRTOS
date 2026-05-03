#include "nerd.h"
#include "nd_internal.h"
#include "nd_lock.h"

nd_thread_t *nd_current_thread = ND_NULL;
nd_thread_t *nd_next_thread = ND_NULL;

nd_uint32_t nd_interrupt_nest_count = 0;
nd_uint32_t nd_interrupt_try_switch = 0;

nd_uint32_t k_thread_ready_bit = 0;
nd_list_t k_thread_ready_list[ND_THREAD_PRIORITY_MAX];

static nd_thread_t k_idle_thread;
ALIGN(8) static nd_uint8_t k_idle_stack[ND_IDLE_STACK_SIZE];

nd_uint64_t last_switch_time;

static void nd_idle_entry(void *param)
{
    (void)param;
    while (1);
}

nd_uint64_t nd_idle_runtime_get(void)
{
    nd_uint64_t idle_runtime = k_idle_thread.usage.total;

    if (nd_current_thread == &k_idle_thread) {
        idle_runtime += nd_hw_get_current() - last_switch_time;
    }

    return idle_runtime;
}

void nd_scheduler_init(void)
{
    nd_thread_list_init();

    size_t offset;

    for (offset = 0; offset < ND_THREAD_PRIORITY_MAX; offset++) {
        nd_list_init(&k_thread_ready_list[offset]);
    }

    nd_thread_create(&k_idle_thread, "idle", nd_idle_entry,
                           ND_THREAD_PRIORITY_MAX - 1, ND_NULL,
                           k_idle_stack, ND_IDLE_STACK_SIZE, ND_THREAD_OPT_ESSENTIAL, 0);
}

void nd_scheduler_start(void)
{
    nd_hw_do_switch_first();
}

void nd_thread_ready_add_tail(nd_thread_t *thread)
{
    nd_size_t prio = thread->priority;

    if (prio >= ND_THREAD_PRIORITY_MAX) {
        return;
    }

    nd_list_insert_before(&k_thread_ready_list[prio], &thread->qnode);

    k_thread_ready_bit |= (1u << prio);
}

void nd_thread_ready_add_head(nd_thread_t *thread)
{
    nd_size_t prio = thread->priority;

    if (prio >= ND_THREAD_PRIORITY_MAX) {
        return;
    }

    nd_list_insert_after(&k_thread_ready_list[prio], &thread->qnode);

    k_thread_ready_bit |= (1u << prio);
}

void nd_thread_ready_remove(nd_thread_t *thread)
{
    nd_size_t prio = thread->priority;

    if (prio >= ND_THREAD_PRIORITY_MAX) {
        return;
    }

    nd_list_remove(&thread->qnode);

    if (nd_list_is_empty(&k_thread_ready_list[prio])) {
        k_thread_ready_bit &= ~(1u << prio);
    }
}

nd_thread_t *nd_get_highest_priority_thread(void)
{
    int highest_priority = nd_ffs(k_thread_ready_bit) - 1;

    if (highest_priority == -1)
        return ND_NULL;
    return nd_list_entry(k_thread_ready_list[highest_priority].next, struct nd_thread, qnode);
}

static nd_bool_t nd_should_switch(nd_thread_t *next, nd_bool_t *rr_rotate)
{
    if (!next || !nd_current_thread) {
        return ND_FALSE;
    }

    *rr_rotate = ND_FALSE;

    if (next->priority < nd_current_thread->priority) {
        return ND_TRUE;
    }

    if (next->priority == nd_current_thread->priority && nd_current_thread->yield) {
        *rr_rotate = ND_TRUE;
        nd_current_thread->yield = 0;
        return ND_TRUE;
    }

    if (nd_current_thread->stat != ND_THREAD_STAT_RUNNING) {
        return ND_TRUE;
    }

    return ND_FALSE;
}

static nd_thread_t *nd_schedule_pick_next(void)
{
    nd_thread_t *next = nd_get_highest_priority_thread();

    if (!next) {
        return ND_NULL;
    }

    nd_bool_t rr_rotate = ND_FALSE;
    if (!nd_should_switch(next, &rr_rotate)) {
        return ND_NULL;
    }

    if (nd_current_thread->stat == ND_THREAD_STAT_RUNNING) {
        nd_bool_t preempted_by_higher = (next->priority < nd_current_thread->priority);

        nd_current_thread->stat = ND_THREAD_STAT_READY;

        if (rr_rotate && nd_current_thread->slice.time_slice) {
            nd_current_thread->slice.slice_left = nd_current_thread->slice.time_slice;
            nd_current_thread->slice.slice_start = 0;
        }

        if (preempted_by_higher) {
            nd_thread_ready_add_head(nd_current_thread);
        } else {
            nd_thread_ready_add_tail(nd_current_thread);
        }
    }

    return next;
}

static void nd_schedule_core(void)
{
    nd_thread_t *next = nd_schedule_pick_next();

    if (!next) {
        return;
    }

    nd_next_thread = next;

    nd_hw_do_switch();
}

void nd_schedule_irq_exit(void)
{
    if (!nd_interrupt_try_switch) {
        return;
    }

    nd_interrupt_try_switch = 0;

    nd_thread_t *next = nd_schedule_pick_next();

    if (!next) return;

    nd_context_switch_cb(next);
}

void nd_context_switch_cb(nd_thread_t *next)
{
    nd_thread_t *prev = nd_current_thread;

    nd_uint64_t now = nd_hw_get_current();

    if (prev) {
        prev->usage.total += now - last_switch_time;
    }

    last_switch_time = now;

    if (next == ND_NULL)
        return;

    nd_thread_ready_remove(next);

    if (prev && prev->slice.time_slice) {
        if (prev->slice.slice_start && prev->slice.slice_left) {
            nd_uint64_t elapsed = now - prev->slice.slice_start;
            if (elapsed >= prev->slice.slice_left) {
                prev->slice.slice_left = 0;
            } else {
                prev->slice.slice_left -= elapsed;
            }
        }
        nd_timer_stop(&prev->slice.slice_timer);
        prev->slice.slice_start = 0;
    }

    nd_next_thread = ND_NULL;
    nd_current_thread = next;

    nd_current_thread->stat = ND_THREAD_STAT_RUNNING;

    if (nd_current_thread->slice.time_slice &&
        nd_current_thread->priority < ND_THREAD_PRIORITY_MAX &&
        !nd_list_is_empty(&k_thread_ready_list[nd_current_thread->priority])) {
        if (nd_current_thread->slice.slice_left == 0 ||
            nd_current_thread->slice.slice_left > nd_current_thread->slice.time_slice) {
            nd_current_thread->slice.slice_left = nd_current_thread->slice.time_slice;
        }
        nd_timer_stop(&nd_current_thread->slice.slice_timer);
        nd_current_thread->slice.slice_timer.timeout = nd_current_thread->slice.slice_left;
        nd_current_thread->slice.slice_start = now;
        nd_timer_start(&nd_current_thread->slice.slice_timer);
    } else if (nd_current_thread->slice.time_slice) {
        nd_timer_stop(&nd_current_thread->slice.slice_timer);

        nd_current_thread->slice.slice_left = nd_current_thread->slice.time_slice;
        nd_current_thread->slice.slice_start = 0;
    }
}

void nd_scheduler(void)
{
    if (nd_interrupt_nest_count > 0) {
        nd_interrupt_try_switch = 1;
        return;
    }

    nd_schedule_core();
}

void nd_thread_slice_timeout(void *arg)
{
    nd_thread_t *thread = (nd_thread_t *)arg;
    if (!thread) {
        return;
    }

    if (thread != nd_current_thread) {
        return;
    }

    if (thread->priority >= ND_THREAD_PRIORITY_MAX ||
        nd_list_is_empty(&k_thread_ready_list[thread->priority])) {
        return;
    }

    thread->yield = 1;
    thread->slice.slice_left = 0;
    thread->slice.slice_start = 0;
    nd_scheduler();
}

void nd_enter_interrupt(void)
{
    nd_kernel_def();
    nd_kernel_lock();
    nd_interrupt_nest_count++;
    nd_kernel_unlock();
}

void nd_exit_interrupt(void)
{
    nd_kernel_def();
    nd_kernel_lock();
    nd_interrupt_nest_count--;
    nd_kernel_unlock();
}

void nd_try_schedule(void)
{
    if (nd_interrupt_nest_count > 0) {
        return;
    }

    if (nd_interrupt_try_switch == 0) {
        return;
    }

    nd_interrupt_try_switch = 0;

    nd_schedule_core();
}

void nd_try_schedule_irqsave(void)
{
    nd_kernel_def();
    nd_kernel_lock();
    nd_try_schedule();
    nd_kernel_unlock();
}

void nd_thread_yield(void)
{
    nd_kernel_def();
    nd_kernel_lock();
    nd_current_thread->yield = 1;
    nd_scheduler();
    nd_kernel_unlock();
}

static void thread_timer_callback(void *arg)
{
    nd_thread_t *thread = (nd_thread_t *)arg;

    thread->stat = ND_THREAD_STAT_READY;
    if (thread->slice.time_slice) {
        thread->slice.slice_left = thread->slice.time_slice;
        thread->slice.slice_start = 0;
    }
    nd_thread_ready_add_tail(thread);

    nd_scheduler();
}

void nd_thread_delay(nd_uint64_t delay)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (nd_current_thread->slice.time_slice) {
        nd_timer_stop(&nd_current_thread->slice.slice_timer);
        nd_current_thread->slice.slice_left = nd_current_thread->slice.time_slice;
        nd_current_thread->slice.slice_start = 0;
    }

    nd_current_thread->stat = ND_THREAD_STAT_BLOCK;
    nd_current_thread->timer.timeout = delay;
    nd_current_thread->timer.callback = thread_timer_callback;
    nd_timer_start(&nd_current_thread->timer);

    nd_scheduler();

    nd_kernel_unlock();
}
