#include "nerd.h"
#include "nd_lock.h"

void nd_mutex_init(nd_mutex_t *mutex)
{
    nd_kernel_def();
    nd_kernel_lock();

    mutex->owner = ND_NULL;
    mutex->priority = ND_THREAD_PRIORITY_MAX;

    nd_list_init(&mutex->wait_list);
    nd_list_init(&mutex->owner_list);

    nd_kernel_unlock();
}

/*
 *  优先级继承：提高优先级
 */
static void nd_thread_change_priority(nd_thread_t *thread, nd_uint8_t new_priority)
{
    if (thread->stat == ND_THREAD_STAT_READY) {
        nd_thread_ready_remove(thread);

        thread->priority = new_priority;

        nd_thread_ready_add_head(thread);
    } else {
        thread->priority = new_priority;
    }
}

/*
 *  判断该线程是否还持有其他更高优先级线程等待的锁
 */
static void nd_mutex_restore_priority(nd_thread_t *thread)
{
    nd_mutex_t *mutex;

    thread->priority = thread->init_priority;

    nd_list_for_each_entry(mutex, &thread->taken_list, owner_list) {
        if (mutex->priority < thread->priority) {
            thread->priority = mutex->priority;
        }
    }
}

nd_err_t nd_mutex_unlock(nd_mutex_t *mutex)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (mutex->owner != nd_current_thread) {
        nd_kernel_unlock();
        return ND_EPERM;
    }

    nd_list_remove(&mutex->owner_list);
    nd_mutex_restore_priority(nd_current_thread);
//链式升级 降级 自我嵌套
    if (nd_list_is_empty(&mutex->wait_list)) {
        mutex->owner = ND_NULL;
        mutex->priority = ND_THREAD_PRIORITY_MAX;
    } else {
        mutex->owner = nd_ipc_resume(&mutex->wait_list);
        nd_list_insert_after(&mutex->owner->taken_list, &mutex->owner_list);
        nd_scheduler();
    }

    nd_kernel_unlock();

    return ND_EOK;
}

nd_err_t nd_mutex_lock(nd_mutex_t *mutex, nd_uint64_t timeout)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (mutex->owner == nd_current_thread) {
        nd_kernel_unlock();
        return ND_EDEADLK;
    }

    if (mutex->owner == ND_NULL) {
        mutex->owner = nd_current_thread;
        nd_list_insert_after(&mutex->owner->taken_list, &mutex->owner_list);
        nd_kernel_unlock();
        return ND_EOK;
    }

    if (mutex->owner->priority > nd_current_thread->priority) {
        mutex->priority = nd_current_thread->priority;
        nd_thread_change_priority(mutex->owner, nd_current_thread->priority);
    }

    if (timeout == ND_TIMEOUT_NOWAIT) {
        nd_kernel_unlock();
        return ND_EBUSY;
    }

    nd_ipc_suspend(&mutex->wait_list, timeout);

    nd_scheduler();

    nd_kernel_unlock();

    return nd_current_thread->error;
}
