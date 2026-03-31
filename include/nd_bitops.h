#ifndef __ND_BITOPS_H__
#define __ND_BITOPS_H__

#include "nd_def.h"

#if defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__) || defined(__GNUC__)

#if defined(__IAR_SYSTEMS_ICC__)
#include <intrinsics.h>
#endif

nd_inline int nd_ffs(int value)
{
    if (value == 0) return 0;
#if defined(__CC_ARM)
    return __clz(__rbit((unsigned int)value)) + 1;
#elif defined(__IAR_SYSTEMS_ICC__)
    return __CLZ(__RBIT((unsigned int)value)) + 1;
#else
    return __builtin_ctz((unsigned int)value) + 1;
#endif
}

#else

static const nd_uint8_t _nd_lowest_bit[] = {
    /* 00 */ 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 10 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 20 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 30 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 40 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 50 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 60 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 70 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 80 */ 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 90 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* A0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* B0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* C0 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* D0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* E0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* F0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

static int nd_ffs(int value)
{
    if (value == 0) return 0;
    if (value & 0xff)     return _nd_lowest_bit[value & 0xff] + 1;
    if (value & 0xff00)   return _nd_lowest_bit[(value & 0xff00) >> 8] + 9;
    if (value & 0xff0000) return _nd_lowest_bit[(value & 0xff0000) >> 16] + 17;
    return _nd_lowest_bit[(value & 0xff000000) >> 24] + 25;
}

#endif

#endif /* __ND_BITOPS_H__ */
