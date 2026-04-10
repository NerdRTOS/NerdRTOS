#ifndef __ND_IPC_H__
#define __ND_IPC_H__

#include "nd_def.h"
#include "nd_list.h"
#include "nd_thread.h"

typedef struct nd_sem {
    nd_int32_t val;

    nd_list_t wait_list;
} nd_sem_t;

typedef struct nd_mutex {
    nd_uint8_t  priority;

    nd_thread_t *owner;

    nd_list_t   wait_list;
    nd_list_t   owner_list;  /* 链表节点，用于挂到 owner 线程的 taken_list 链表上 */
} nd_mutex_t;

typedef struct nd_queue {
    char         *buf;
    nd_uint32_t  msg_size;
    nd_uint32_t  max_msgs;
    nd_uint32_t  used_msg;
    nd_uint32_t  read_ptr;
    nd_uint32_t  write_ptr;

    nd_list_t   send_wait;
    nd_list_t   read_wait;
} nd_queue_t;

typedef struct nd_event {
    nd_uint64_t set;

    nd_list_t   wait_list;
} nd_event_t;

void nd_sem_init(nd_sem_t *sem, nd_int32_t init_val);
nd_err_t nd_sem_take(nd_sem_t *sem, nd_uint64_t timeout);
nd_err_t nd_sem_release(nd_sem_t *sem);

void nd_mutex_init(nd_mutex_t *mutex);
nd_err_t nd_mutex_lock(nd_mutex_t *mutex, nd_uint64_t timeout);
nd_err_t nd_mutex_unlock(nd_mutex_t *mutex);

nd_err_t nd_queue_init(nd_queue_t *queue, char *buf, nd_uint32_t msg_size, nd_uint32_t max_msgs);
nd_err_t nd_queue_send(nd_queue_t *queue, void *c, nd_uint64_t timeout);
nd_err_t nd_queue_recv(nd_queue_t *queue, void *buf, nd_uint64_t timeout);

void nd_event_init(nd_event_t *event);
nd_err_t nd_event_send(nd_event_t *event, nd_uint32_t set);
nd_err_t nd_event_recv(nd_event_t *event, nd_uint32_t set, nd_event_opt_t opt, nd_uint64_t timeout);

#endif
