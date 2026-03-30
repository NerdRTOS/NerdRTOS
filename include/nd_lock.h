#ifndef __ND_LOCK_H__
#define __ND_LOCK_H__

#include "nd_hw.h"

#define nd_kernel_def()   nd_size_t status
#define nd_kernel_lock()  status = nd_hw_irq_save()
#define nd_kernel_unlock() nd_hw_irq_restore(status)

#endif /* __ND_LOCK_H__ */
