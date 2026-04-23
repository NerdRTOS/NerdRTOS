#ifndef __ND_CPULOAD_H__
#define __ND_CPULOAD_H__

#include "nd_thread.h"

nd_uint32_t nd_cpuload_get(void);
nd_uint32_t nd_cpuload_thread_get(nd_thread_t *thread);

#endif /* __ND_CPULOAD_H__ */
