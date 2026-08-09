#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include "nd_hw.h"
#include "nd_shell.h"

#define BYTE_ORDER                  LITTLE_ENDIAN

#define LWIP_PROVIDE_ERRNO          1

#define LWIP_PLATFORM_DIAG(message) \
    do {                            \
        shell_printf message;       \
    } while (0)

#define LWIP_PLATFORM_ASSERT(message)                              \
    do {                                                           \
        shell_printf("lwIP assertion failed: %s (%s:%d)\r\n",      \
                     message, __FILE__, __LINE__);                 \
        nd_hw_irq_disable();                                       \
        for (;;) {                                                 \
        }                                                          \
    } while (0)

#endif /* LWIP_ARCH_CC_H */
