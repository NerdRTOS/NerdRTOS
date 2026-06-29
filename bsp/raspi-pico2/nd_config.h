#ifndef __ND_CONFIG_H__
#define __ND_CONFIG_H__

#include <autoconf.h>

#define ND_CPU_CLOCK_HZ     CONFIG_ND_CPU_CLOCK_HZ

#define ND_NAME_MAX_SIZE    CONFIG_ND_NAME_MAX_SIZE

#define ND_TICKLESS_FREQ    CONFIG_ND_TICKLESS_FREQ

#define ND_TICKS_PER_SEC    CONFIG_ND_TICKS_PER_SEC

#define ND_IDLE_STACK_SIZE  CONFIG_ND_IDLE_STACK_SIZE

#ifdef CONFIG_ND_CFG_TICKLESS
    #define ND_CFG_TICKLESS     Y
#else
    #define ND_CFG_TICKLESS     N
#endif

#ifdef CONFIG_ND_CFG_DEBUG
    #define ND_CFG_DEBUG        Y
#else
    #define ND_CFG_DEBUG        N
#endif

#endif /* __ND_CONFIG_H__ */
