#include "nd_hw.h"
#include "nd_def.h"
#include "nd_list.h"
#include "nd_thread.h"
#include "nd_lock.h"
#include "nd_timer.h"
#include "nerd.h"

nd_thread_t *nd_current_thread = ND_NULL;
nd_thread_t *nd_next_thread = ND_NULL;

nd_uint32_t nd_interrupt_nest_count = 0;
nd_uint32_t nd_interrupt_try_switch = 0;

nd_uint32_t k_thread_ready_bit = 0;
nd_list_t k_thread_ready_list[ND_THREAD_PRIORITY_MAX];

static nd_thread_t k_idle_thread;
ALIGN(8) static nd_uint8_t k_idle_stack[ND_IDLE_STACK_SIZE];

static void nd_idle_entry(void *param)
{
    (void)param;
    while (1);
}

const nd_uint8_t _lowest_bit_bitmap[] = {
    /* 00 */ 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 10 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 20 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 30 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 40 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 50 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 60 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 70 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 80 */ 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 90 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* A0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* B0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* C0 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* D0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* E0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* F0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

static int _ffs(int value)
{
    if (value == 0) return 0;

    if (value & 0xff)
        return _lowest_bit_bitmap[value & 0xff] + 1;

    if (value & 0xff00)
        return _lowest_bit_bitmap[(value & 0xff00) >> 8] + 9;

    if (value & 0xff0000)
        return _lowest_bit_bitmap[(value & 0xff0000) >> 16] + 17;

    return _lowest_bit_bitmap[(value & 0xff000000) >> 24] + 25;
}

/*
 * 调度器要点：
 * - 就绪队列：每个优先级一个链表 + 位图快速查找最高优先级。
 * - ISR 内不直接切换：在中断上下文仅置位 nd_interrupt_try_switch，
 *   由 nd_try_schedule()（中断退出后）完成真实切换。
 * - 同优先级 RR：时间片到期，将当前线程插入队尾。
 * - 主动 yield 时，将当前线程插入队尾。
 * - 被更高优先级抢占：将当前线程插入队首，并保留剩余时间片 slice_left。
 */
void nd_scheduler_init(void)
{
    nd_thread_list_init();

    size_t offset;

    for (offset = 0; offset < ND_THREAD_PRIORITY_MAX; offset++) {
        nd_list_init(&k_thread_ready_list[offset]);
    }

    nd_thread_init(&k_idle_thread, "idle", nd_idle_entry,
                   ND_THREAD_PRIORITY_MAX - 1, ND_NULL,
                   k_idle_stack, ND_IDLE_STACK_SIZE, 0);
}

void nd_scheduler_start(void)
{
    nd_hw_do_switch_first();
}

/*
 * 将线程加入就绪队列尾部。
 */
void nd_thread_ready_add_tail(nd_thread_t *thread)
{
    nd_size_t prio = thread->priority;

    if (prio >= ND_THREAD_PRIORITY_MAX) {
        return;
    }

    nd_list_insert_before(&k_thread_ready_list[prio], &thread->prio_list);

    k_thread_ready_bit |= (1u << prio);
}

/* 将线程加入就绪队列头部（用于被高优先级抢占后尽量“续跑”）。 */
void nd_thread_ready_add_head(nd_thread_t *thread)
{
    nd_size_t prio = thread->priority;

    if (prio >= ND_THREAD_PRIORITY_MAX) {
        return;
    }

    nd_list_insert_after(&k_thread_ready_list[prio], &thread->prio_list);

    k_thread_ready_bit |= (1u << prio);
}

/* 将线程从就绪队列移除；若该优先级队列空则清位图。 */
void nd_thread_ready_remove(nd_thread_t *thread)
{
    nd_size_t prio = thread->priority;

    if (prio >= ND_THREAD_PRIORITY_MAX) {
        return;
    }

    nd_list_remove(&thread->prio_list);

    if (nd_list_is_empty(&k_thread_ready_list[prio])) {
        k_thread_ready_bit &= ~(1u << prio);
    }
}

nd_thread_t *nd_get_highest_priority_thread(void)
{
    int highest_priority = _ffs(k_thread_ready_bit) - 1;

    if (highest_priority == -1)
        return ND_NULL;
    return nd_list_entry(k_thread_ready_list[highest_priority].next, struct nd_thread, prio_list);
}

/*
 * 判断是否需要切换到 next。
 * rr_rotate = TRUE 表示“同优先级 RR 轮转”（由 yield/时间片到期触发）。
 * 注意：一旦消费 yield，这里会清掉 yield，避免重复触发。
 */
static nd_bool_t nd_should_switch(nd_thread_t *next, nd_bool_t *rr_rotate)
{
    if (!next || !nd_current_thread) {
        return ND_FALSE;
    }

    *rr_rotate = ND_FALSE;

    /* 更高优先级线程到来：必须切走 */
    if (next->priority < nd_current_thread->priority) {
        return ND_TRUE;
    }

    /* 同优先级 RR 轮转：切走 */
    if (next->priority == nd_current_thread->priority && nd_current_thread->yield) {
        *rr_rotate = ND_TRUE;
        nd_current_thread->yield = 0;
        return ND_TRUE;
    }

    /* 当前线程不是RUNNING：必须切走 */
    if (nd_current_thread->stat != ND_THREAD_STAT_RUNNING) {
        return ND_TRUE;
    }

    return ND_FALSE;
}

/*
 * 调度核心：选出 next、判定是否需要切换、处理 current 入队，然后触发切换。
 * 前提：不得在 ISR 嵌套中调用（nd_scheduler/nd_try_schedule 已保证）。
 */
static void nd_schedule_core(void)
{
    nd_thread_t *next = nd_get_highest_priority_thread();
    if (!next) {
        return;
    }

    nd_bool_t rr_rotate = ND_FALSE;
    if (!nd_should_switch(next, &rr_rotate)) {
        return;
    }

    /*
     * 把 current 线程按策略放回就绪队列。
     */
    if (nd_current_thread->stat == ND_THREAD_STAT_RUNNING) {
        /*
         * 入队策略：
         * - 同优先级 RR：入队尾部
         * - 被更高优先级抢占：入队头部（减少周期性高优先级线程打断造成的偏置）
         */
        nd_bool_t preempted_by_higher = (next->priority < nd_current_thread->priority);

        nd_current_thread->stat = ND_THREAD_STAT_READY;

        /* RR 轮转：切出后给下一轮补满时间片 */
        if (rr_rotate && nd_current_thread->time_slice) {
            nd_current_thread->slice_left = nd_current_thread->time_slice;
            nd_current_thread->slice_start = 0;
        }

        if (preempted_by_higher) {
            nd_thread_ready_add_head(nd_current_thread);
        } else {
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

    /* 从就绪队列中摘除 next（如果在就绪队列中） */
    nd_thread_ready_remove(next);

    /*
     * 时间片剩余语义：
     * - 切出时按实际运行时间扣减 slice_left（被高优先级抢占时保留剩余配额）
     * - 同优先级轮转时由调度核心把 slice_left 重置为满额
     */
    if (prev && prev->time_slice) {
        if (prev->slice_start && prev->slice_left) {
            nd_uint64_t elapsed = now - prev->slice_start;
            if (elapsed >= prev->slice_left) {
                prev->slice_left = 0;
            } else {
                prev->slice_left -= elapsed;
            }
        }
        nd_timer_stop(&prev->slice_timer);
        prev->slice_start = 0;
    }

    /* 设置为当前线程，并清除临时 next 指针 */
    nd_next_thread = ND_NULL;
    nd_current_thread = next;

    nd_current_thread->stat = ND_THREAD_STAT_RUNNING;

    /* 启动当前线程的时间片定时器：仅当存在同优先级就绪线程时才启用 */
    if (nd_current_thread->time_slice &&
        nd_current_thread->priority < ND_THREAD_PRIORITY_MAX &&
        !nd_list_is_empty(&k_thread_ready_list[nd_current_thread->priority])) {
        if (nd_current_thread->slice_left == 0 ||
            nd_current_thread->slice_left > nd_current_thread->time_slice) {
            nd_current_thread->slice_left = nd_current_thread->time_slice;
        }
        nd_timer_stop(&nd_current_thread->slice_timer);
        nd_current_thread->slice_timer.timeout = nd_current_thread->slice_left;
        nd_current_thread->slice_start = now;
        nd_timer_start(&nd_current_thread->slice_timer);
    } else if (nd_current_thread->time_slice) {
        nd_timer_stop(&nd_current_thread->slice_timer);
        /* 没有同优先级竞争者时，重新给满额度，避免下次竞争时带着“残余配额” */
        nd_current_thread->slice_left = nd_current_thread->time_slice;
        nd_current_thread->slice_start = 0;
    }
}

void nd_scheduler(void)
{
    if (nd_interrupt_nest_count > 0) {
        /* ISR 内仅置位 */
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

    /* 只对“当前正在运行”的线程生效 */
    if (thread != nd_current_thread) {
        return;
    }

    /* 没有同优先级竞争者时，不做轮转 */
    if (thread->priority >= ND_THREAD_PRIORITY_MAX ||
        nd_list_is_empty(&k_thread_ready_list[thread->priority])) {
        return;
    }

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
    if (thread->time_slice) {
        thread->slice_left = thread->time_slice;
        thread->slice_start = 0;
    }
    nd_thread_ready_add_tail(thread);

    nd_scheduler();
}

void nd_thread_delay(nd_uint64_t delay)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (nd_current_thread->time_slice) {
        nd_timer_stop(&nd_current_thread->slice_timer);
        nd_current_thread->slice_left = nd_current_thread->time_slice;
        nd_current_thread->slice_start = 0;
    }

    nd_current_thread->stat = ND_THREAD_STAT_BLOCK;
    nd_current_thread->timer.timeout = delay;
    nd_current_thread->timer.callback = thread_timer_callback;
    nd_timer_start(&nd_current_thread->timer);

    nd_scheduler();

    nd_kernel_unlock();
}
