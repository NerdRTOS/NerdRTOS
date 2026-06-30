#ifndef __PTIMER_H__
#define __PTIMER_H__

#include "nd_def.h"

#define PTIMER_BASE                 0x1E000600
#define PTIMER_LOAD                 (*(volatile nd_uint32_t *)(PTIMER_BASE + 0x000))
#define PTIMER_COUNTER              (*(volatile nd_uint32_t *)(PTIMER_BASE + 0x004))
#define PTIMER_CTRL                 (*(volatile nd_uint32_t *)(PTIMER_BASE + 0x008))
#define PTIMER_ISR                  (*(volatile nd_uint32_t *)(PTIMER_BASE + 0x00C))

#define PTIMER_CTRL_ENABLE          (1U << 0)
#define PTIMER_CTRL_AUTO_RELOAD     (1U << 1)
#define PTIMER_CTRL_IRQ_ENABLE      (1U << 2)

#define PTIMER_ISR_EVENT            (1U << 0)

#define GTIMER_BASE                 0x1E000200
#define GTIMER_COUNT_LO             (*(volatile nd_uint32_t *)(GTIMER_BASE + 0x000))
#define GTIMER_COUNT_HI             (*(volatile nd_uint32_t *)(GTIMER_BASE + 0x004))
#define GTIMER_CTRL                 (*(volatile nd_uint32_t *)(GTIMER_BASE + 0x008))

#define GTIMER_CTRL_ENABLE          (1U << 0)

#endif /* __PTIMER_H__ */
