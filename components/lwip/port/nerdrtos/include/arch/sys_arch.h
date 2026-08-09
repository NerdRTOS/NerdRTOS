#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "nd_def.h"
#include "nd_ipc.h"
#include "nd_thread.h"

#define SYS_ARCH_MBOX_CAPACITY    16U

typedef struct {
    nd_sem_t  sem;
    nd_uint8_t valid;
} sys_sem_t;

typedef struct {
    nd_mutex_t mutex;
    nd_uint8_t valid;
} sys_mutex_t;

typedef struct {
    nd_queue_t queue;
    void      *messages[SYS_ARCH_MBOX_CAPACITY];
    nd_uint8_t valid;
} sys_mbox_t;

typedef nd_size_t sys_prot_t;
typedef nd_thread_t *sys_thread_t;

#endif /* LWIP_ARCH_SYS_ARCH_H */
