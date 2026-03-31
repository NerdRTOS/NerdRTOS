#include "nerd.h"
#include "nd_lock.h"

void nd_sem_init(nd_sem_t *sem, nd_int32_t init_val)
{
    nd_kernel_def();
    nd_kernel_lock();

    sem->val = init_val;

    nd_list_init(&sem->wait_list);

    nd_kernel_unlock();
}

nd_err_t nd_sem_release(nd_sem_t *sem)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (nd_list_is_empty(&sem->wait_list)) {
        sem->val++;
    } else {
        nd_ipc_resume(&sem->wait_list);

        nd_scheduler();
    }

    nd_kernel_unlock();

    return ND_EOK;
}

nd_err_t nd_sem_take(nd_sem_t *sem, nd_uint64_t timeout)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (sem->val > 0) {
        sem->val--;
        nd_kernel_unlock();
        return ND_EOK;
    }

    if (timeout == ND_TIMEOUT_NOWAIT) {
        nd_kernel_unlock();
        return ND_EBUSY;
    }

    nd_ipc_suspend(&sem->wait_list, timeout);

    nd_scheduler();

    nd_kernel_unlock();

    return nd_current_thread->error;
}
