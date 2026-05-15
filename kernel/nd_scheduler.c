#include "nerd.h"
#include "nd_lock.h"
#include <stdio.h>
nd_thread_t *nd_current_thread = ND_NULL;
nd_thread_t *nd_next_thread = ND_NULL;

nd_uint32_t nd_interrupt_nest_count = 0;
nd_uint32_t nd_interrupt_try_switch = 0;

nd_uint32_t k_thread_ready_bit = 0;
nd_list_t k_thread_ready_list[ND_THREAD_PRIORITY_MAX];

static nd_thread_t k_idle_thread;
ALIGN(8)
static nd_uint8_t k_idle_stack[ND_IDLE_STACK_SIZE];

static void nd_idle_entry(void *param)
{
    (void)param;
    while (1)
    {
        __asm__ volatile("wfi");
    }
}

void nd_scheduler_init(void)
{
    nd_thread_list_init();

    size_t offset;
    for (offset = 0; offset < ND_THREAD_PRIORITY_MAX; offset++)
    {
        nd_list_init(&k_thread_ready_list[offset]);
    }

    nd_thread_init(&k_idle_thread, "idle", nd_idle_entry,
                   ND_THREAD_PRIORITY_MAX - 1, ND_NULL,
                   k_idle_stack, ND_IDLE_STACK_SIZE, 0);

    nd_thread_ready_add_tail(&k_idle_thread);
}

void nd_scheduler_start(void)
{
    nd_hw_do_switch_first();
}

void nd_thread_ready_add_tail(nd_thread_t *thread)
{
    nd_size_t prio = thread->priority;
    if (prio >= ND_THREAD_PRIORITY_MAX)
        return;
    nd_list_insert_before(&k_thread_ready_list[prio], &thread->prio_list);
    k_thread_ready_bit |= (1u << prio);
}

void nd_thread_ready_add_head(nd_thread_t *thread)
{
    nd_size_t prio = thread->priority;
    if (prio >= ND_THREAD_PRIORITY_MAX)
        return;
    nd_list_insert_after(&k_thread_ready_list[prio], &thread->prio_list);
    k_thread_ready_bit |= (1u << prio);
}

static nd_bool_t nd_thread_is_ready_listed(nd_thread_t *thread)
{
    return !(thread->prio_list.next == &thread->prio_list &&
             thread->prio_list.prev == &thread->prio_list);
}

void nd_thread_ready_remove(nd_thread_t *thread)
{
    nd_size_t prio = thread->priority;

    if (prio >= ND_THREAD_PRIORITY_MAX)
    {
        return;
    }

    if (!nd_thread_is_ready_listed(thread))
    {
        return;
    }

    nd_list_remove(&thread->prio_list);
    nd_list_init(&thread->prio_list);

    if (nd_list_is_empty(&k_thread_ready_list[prio]))
    {
        k_thread_ready_bit &= ~(1u << prio);
    }
}

nd_thread_t *nd_get_highest_priority_thread(void)
{
    int highest_priority = nd_ffs(k_thread_ready_bit) - 1;
    if (highest_priority == -1)
        return ND_NULL;
    return nd_list_entry(k_thread_ready_list[highest_priority].next,
                         struct nd_thread, prio_list);
}

static nd_bool_t nd_should_switch(nd_thread_t *next, nd_bool_t *rr_rotate)
{
    if (!next || !nd_current_thread)
        return ND_FALSE;
    *rr_rotate = ND_FALSE;
    if (next->priority < nd_current_thread->priority)
        return ND_TRUE;
    if (next->priority == nd_current_thread->priority &&
        nd_current_thread->yield)
    {
        *rr_rotate = ND_TRUE;
        nd_current_thread->yield = 0;
        return ND_TRUE;
    }
    if (nd_current_thread->stat != ND_THREAD_STAT_RUNNING)
        return ND_TRUE;
    return ND_FALSE;
}

static void nd_schedule_core(void)
{
    nd_thread_t *next = nd_get_highest_priority_thread();

    if (!next)
    {
        while (1)
        {
            __asm__ volatile("wfi");
        }
    }

    if (nd_current_thread &&
        nd_current_thread->stat != ND_THREAD_STAT_RUNNING &&
        next == nd_current_thread)
    {
        // printf("schedule error: blocked current thread is still in ready list\r\n");
        while (1)
        {
            __asm__ volatile("wfi");
        }
    }

    nd_bool_t rr_rotate = ND_FALSE;
    if (!nd_should_switch(next, &rr_rotate))
    {
        return;
    }

    if (nd_current_thread &&
        nd_current_thread->stat == ND_THREAD_STAT_RUNNING)
    {
        nd_bool_t preempted_by_higher =
            (next->priority < nd_current_thread->priority);

        nd_current_thread->stat = ND_THREAD_STAT_READY;

        if (rr_rotate && nd_current_thread->time_slice)
        {
            nd_current_thread->slice_left = nd_current_thread->time_slice;
            nd_current_thread->slice_start = 0;
        }

        if (preempted_by_higher)
        {
            nd_thread_ready_add_head(nd_current_thread);
        }
        else
        {
            nd_thread_ready_add_tail(nd_current_thread);
        }
    }

    nd_next_thread = next;
    nd_hw_do_switch();
}

void nd_context_switch_cb(nd_thread_t *next)
{
    nd_thread_t *prev = nd_current_thread;
    nd_uint64_t now = nd_hw_get_current();

    if (next == ND_NULL)
        return;

    nd_thread_ready_remove(next);

    if (prev && prev->time_slice)
    {
        if (prev->slice_start && prev->slice_left)
        {
            nd_uint64_t elapsed = now - prev->slice_start;
            if (elapsed >= prev->slice_left)
                prev->slice_left = 0;
            else
                prev->slice_left -= elapsed;
        }
        nd_timer_stop(&prev->slice_timer);
        prev->slice_start = 0;
    }

    nd_next_thread = ND_NULL;
    nd_current_thread = next;
    nd_current_thread->stat = ND_THREAD_STAT_RUNNING;

    if (nd_current_thread->time_slice &&
        nd_current_thread->priority < ND_THREAD_PRIORITY_MAX &&
        !nd_list_is_empty(&k_thread_ready_list[nd_current_thread->priority]))
    {
        if (nd_current_thread->slice_left == 0 ||
            nd_current_thread->slice_left > nd_current_thread->time_slice)
            nd_current_thread->slice_left = nd_current_thread->time_slice;
        nd_timer_stop(&nd_current_thread->slice_timer);
        nd_current_thread->slice_timer.timeout = nd_current_thread->slice_left;
        nd_current_thread->slice_start = now;
        nd_timer_start(&nd_current_thread->slice_timer);
    }
    else if (nd_current_thread->time_slice)
    {
        nd_timer_stop(&nd_current_thread->slice_timer);
        nd_current_thread->slice_left = nd_current_thread->time_slice;
        nd_current_thread->slice_start = 0;
    }
}

void nd_scheduler(void)
{
    if (nd_interrupt_nest_count > 0)
    {
        nd_interrupt_try_switch = 1;
        return;
    }
    nd_schedule_core();
}

void nd_thread_slice_timeout(void *arg)
{
    nd_thread_t *thread = (nd_thread_t *)arg;
    if (!thread || thread != nd_current_thread)
        return;
    if (thread->priority >= ND_THREAD_PRIORITY_MAX ||
        nd_list_is_empty(&k_thread_ready_list[thread->priority]))
        return;
    thread->yield = 1;
    thread->slice_left = 0;
    thread->slice_start = 0;
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
    if (nd_interrupt_nest_count > 0)
        return;
    if (nd_interrupt_try_switch == 0)
        return;
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
    nd_kernel_unlock();

    nd_scheduler();
}

static void thread_timer_callback(void *arg)
{
    nd_thread_t *thread = (nd_thread_t *)arg;
    thread->stat = ND_THREAD_STAT_READY;
    if (thread->time_slice)
    {
        thread->slice_left = thread->time_slice;
        thread->slice_start = 0;
    }
    nd_thread_ready_add_tail(thread);
    nd_scheduler();
}

void nd_thread_delay(nd_uint64_t delay)
{
    nd_kernel_def();

    if (delay == 0)
    {
        nd_thread_yield();
        return;
    }

    nd_kernel_lock();

    nd_current_thread->stat = ND_THREAD_STAT_BLOCK;

    nd_current_thread->timer.type = ND_TIMER_TYPE_ONE_SHOT;
    nd_current_thread->timer.timeout = delay;
    nd_current_thread->timer.callback = thread_timer_callback;
    nd_current_thread->timer.arg = nd_current_thread;

    nd_timer_start(&nd_current_thread->timer);

    nd_kernel_unlock();

    nd_scheduler();
}
