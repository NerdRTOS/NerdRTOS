#include "nd_ipc.h"

nd_err_t nd_queue_init(nd_queue_t *queue, char *buf, nd_uint32_t msg_size, nd_uint32_t max_msgs)
{
    nd_kernel_def();
    nd_kernel_lock();

    if ((max_msgs & (max_msgs-1)) != 0 || max_msgs < 1) {
        return ND_EINVAL;
    }

    queue->buf = buf;
    queue->msg_size = msg_size;
    queue->max_msgs = max_msgs;
    queue->used_msg = 0;
    queue->write_ptr = 0;
    queue->read_ptr = 0;

    nd_list_init(&queue->read_wait);
    nd_list_init(&queue->send_wait);

    nd_kernel_unlock();

    return ND_EOK;
}

nd_err_t nd_queue_send(nd_queue_t *queue, void *c, nd_uint64_t timeout)
{
    nd_kernel_def();
    nd_kernel_lock();

    while (queue->used_msg == queue->max_msgs) {
        if (timeout == ND_TIMEOUT_NOWAIT) {
            nd_kernel_unlock();
            return ND_EFULL;
        }

        nd_ipc_suspend(&queue->send_wait, timeout);

        nd_scheduler();

        if (nd_current_thread->error == ND_ETIMEOUT) {
            nd_kernel_unlock();

            return nd_current_thread->error;
        }
    }

    nd_memcpy(queue->buf + (queue->write_ptr & (queue->max_msgs - 1)) * queue->msg_size, c, queue->msg_size);

    queue->write_ptr++;
    queue->used_msg++;

    if (!nd_list_is_empty(&queue->read_wait)) {
        nd_ipc_resume(&queue->read_wait);

        nd_scheduler();
    }

    nd_kernel_unlock();

    return ND_EOK;
}

nd_err_t nd_queue_recv(nd_queue_t *queue, void *buf, nd_uint64_t timeout)
{
    nd_kernel_def();
    nd_kernel_lock();

    while (queue->used_msg == 0) {
        if (timeout == ND_TIMEOUT_NOWAIT) {
            nd_kernel_unlock();
            return ND_EEMPTY;
        }

        nd_ipc_suspend(&queue->read_wait, timeout);

        nd_scheduler();

        if (nd_current_thread->error == ND_ETIMEOUT) {
            nd_kernel_unlock();

            return nd_current_thread->error;
        }
    }

    nd_memcpy(buf, queue->buf + (queue->read_ptr & (queue->max_msgs - 1)) * queue->msg_size, queue->msg_size);

    queue->read_ptr++;
    queue->used_msg--;

    if (!nd_list_is_empty(&queue->send_wait)) {
        nd_ipc_resume(&queue->send_wait);

        nd_scheduler();
    }

    nd_kernel_unlock();

    return ND_EOK;
}
