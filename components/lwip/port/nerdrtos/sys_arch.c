#include "lwip/opt.h"

#if !NO_SYS

#include "lwip/sys.h"

#include "nerd.h"
#include "nd_internal.h"

#define SYS_MILLISECONDS_PER_SECOND    (1000ULL)
#define SYS_THREAD_ALIGNMENT           (8U)

static nd_uint64_t sys_timeout_to_ticks(u32_t timeout_ms)
{
#if ND_CFG_TICKLESS
    return ((nd_uint64_t)timeout_ms * ND_TICKLESS_FREQ +
            SYS_MILLISECONDS_PER_SECOND - 1ULL) /
           SYS_MILLISECONDS_PER_SECOND;
#else
    return ((nd_uint64_t)timeout_ms * ND_TICKS_PER_SEC +
            SYS_MILLISECONDS_PER_SECOND - 1ULL) /
           SYS_MILLISECONDS_PER_SECOND;
#endif
}

void sys_init(void)
{
}

u32_t sys_now(void)
{
#if ND_CFG_TICKLESS
    return (u32_t)(nd_hw_get_current() * SYS_MILLISECONDS_PER_SECOND /
                   ND_TICKLESS_FREQ);
#else
    return (u32_t)(nd_hw_get_current() * SYS_MILLISECONDS_PER_SECOND /
                   ND_TICKS_PER_SEC);
#endif
}

u32_t sys_jiffies(void)
{
    return (u32_t)nd_hw_get_current();
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    if (sem == NULL || count > 1U) {
        return ERR_ARG;
    }

    nd_sem_init(&sem->sem, count);
    sem->valid = 1U;

    return ERR_OK;
}

void sys_sem_signal(sys_sem_t *sem)
{
    LWIP_ASSERT("sys_sem_signal: invalid semaphore",
                sem != NULL && sem->valid != 0U);
    (void)nd_sem_release(&sem->sem);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    u32_t start;
    nd_err_t result;

    LWIP_ASSERT("sys_arch_sem_wait: invalid semaphore",
                sem != NULL && sem->valid != 0U);

    start = sys_now();
    result = nd_sem_take(&sem->sem,
                         timeout == 0U
                             ? ND_TIMEOUT_FOREVER
                             : sys_timeout_to_ticks(timeout));

    return result == ND_ETIMEOUT ? SYS_ARCH_TIMEOUT : sys_now() - start;
}

void sys_sem_free(sys_sem_t *sem)
{
    LWIP_ASSERT("sys_sem_free: invalid semaphore",
                sem != NULL && sem->valid != 0U);
    sem->valid = 0U;
}

int sys_sem_valid(sys_sem_t *sem)
{
    return sem != NULL && sem->valid != 0U;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
    if (sem != NULL) {
        sem->valid = 0U;
    }
}

err_t sys_mutex_new(sys_mutex_t *mutex)
{
    if (mutex == NULL) {
        return ERR_ARG;
    }

    nd_mutex_init(&mutex->mutex);
    mutex->valid = 1U;

    return ERR_OK;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
    LWIP_ASSERT("sys_mutex_lock: invalid mutex",
                mutex != NULL && mutex->valid != 0U);
    LWIP_ASSERT("sys_mutex_lock: lock failed",
                nd_mutex_lock(&mutex->mutex, ND_TIMEOUT_FOREVER) == ND_EOK);
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
    LWIP_ASSERT("sys_mutex_unlock: invalid mutex",
                mutex != NULL && mutex->valid != 0U);
    LWIP_ASSERT("sys_mutex_unlock: unlock failed",
                nd_mutex_unlock(&mutex->mutex) == ND_EOK);
}

void sys_mutex_free(sys_mutex_t *mutex)
{
    LWIP_ASSERT("sys_mutex_free: invalid mutex",
                mutex != NULL && mutex->valid != 0U);
    mutex->valid = 0U;
}

int sys_mutex_valid(sys_mutex_t *mutex)
{
    return mutex != NULL && mutex->valid != 0U;
}

void sys_mutex_set_invalid(sys_mutex_t *mutex)
{
    if (mutex != NULL) {
        mutex->valid = 0U;
    }
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    if (mbox == NULL || size < 1 ||
        size > (int)SYS_ARCH_MBOX_CAPACITY ||
        (size & (size - 1)) != 0) {
        return ERR_ARG;
    }

    if (nd_queue_init(&mbox->queue, (char *)mbox->messages,
                      sizeof(mbox->messages[0]), (nd_uint32_t)size) != ND_EOK) {
        return ERR_MEM;
    }

    mbox->valid = 1U;

    return ERR_OK;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    LWIP_ASSERT("sys_mbox_post: invalid mailbox",
                mbox != NULL && mbox->valid != 0U);
    LWIP_ASSERT("sys_mbox_post: post failed",
                nd_queue_send(&mbox->queue, &msg, ND_TIMEOUT_FOREVER) == ND_EOK);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    LWIP_ASSERT("sys_mbox_trypost: invalid mailbox",
                mbox != NULL && mbox->valid != 0U);

    return nd_queue_send(&mbox->queue, &msg, ND_TIMEOUT_NOWAIT) == ND_EOK
               ? ERR_OK
               : ERR_MEM;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    return sys_mbox_trypost(mbox, msg);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    void *message;
    u32_t start;
    nd_err_t result;

    LWIP_ASSERT("sys_arch_mbox_fetch: invalid mailbox",
                mbox != NULL && mbox->valid != 0U);

    start = sys_now();
    result = nd_queue_recv(&mbox->queue, &message,
                           timeout == 0U
                               ? ND_TIMEOUT_FOREVER
                               : sys_timeout_to_ticks(timeout));
    if (result == ND_ETIMEOUT) {
        return SYS_ARCH_TIMEOUT;
    }
    if (result != ND_EOK) {
        return SYS_ARCH_TIMEOUT;
    }

    if (msg != NULL) {
        *msg = message;
    }

    return sys_now() - start;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    void *message;

    LWIP_ASSERT("sys_arch_mbox_tryfetch: invalid mailbox",
                mbox != NULL && mbox->valid != 0U);

    if (nd_queue_recv(&mbox->queue, &message, ND_TIMEOUT_NOWAIT) != ND_EOK) {
        return SYS_MBOX_EMPTY;
    }

    if (msg != NULL) {
        *msg = message;
    }

    return 0U;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
    LWIP_ASSERT("sys_mbox_free: invalid mailbox",
                mbox != NULL && mbox->valid != 0U);
    LWIP_ASSERT("sys_mbox_free: mailbox still contains messages",
                mbox->queue.used_msg == 0U);
    mbox->valid = 0U;
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
    return mbox != NULL && mbox->valid != 0U;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
    if (mbox != NULL) {
        mbox->valid = 0U;
    }
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread,
                            void *arg, int stacksize, int prio)
{
    nd_uint8_t *allocation;
    nd_uint8_t *stack;
    nd_thread_t *new_thread;
    nd_uint32_t thread_size;
    nd_uint32_t allocation_size;

    LWIP_ASSERT("sys_thread_new: invalid entry", thread != NULL);
    LWIP_ASSERT("sys_thread_new: invalid stack size", stacksize > 0);
    LWIP_ASSERT("sys_thread_new: invalid priority",
                prio >= 0 && prio < ND_THREAD_PRIORITY_MAX);

    thread_size = ND_ALIGN(sizeof(nd_thread_t), SYS_THREAD_ALIGNMENT);
    allocation_size = thread_size + (nd_uint32_t)stacksize +
                      SYS_THREAD_ALIGNMENT - 1U;
    allocation = nd_malloc(allocation_size);
    LWIP_ASSERT("sys_thread_new: out of memory", allocation != NULL);
    if (allocation == NULL) {
        return ND_NULL;
    }

    new_thread = (nd_thread_t *)ND_ALIGN((nd_ubase_t)allocation,
                                         SYS_THREAD_ALIGNMENT);
    stack = (nd_uint8_t *)new_thread + thread_size;

    if (nd_thread_create(new_thread, name, thread, (nd_uint8_t)prio,
                         arg, stack, (nd_size_t)stacksize,
                         ND_THREAD_OPT_NONE, 0U) != ND_EOK) {
        nd_free(allocation);
        LWIP_ASSERT("sys_thread_new: thread creation failed", 0);
        return ND_NULL;
    }

    return new_thread;
}

sys_prot_t sys_arch_protect(void)
{
    return nd_hw_irq_save();
}

void sys_arch_unprotect(sys_prot_t protection)
{
    nd_hw_irq_restore(protection);
}

#endif /* !NO_SYS */
