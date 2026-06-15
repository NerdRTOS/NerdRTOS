#ifndef __ND_ASSERT_H__
#define __ND_ASSERT_H__

#include "nd_def.h"

#define ND_ASSERT_BUF_SIZE              160

void nd_assert_puts(const char *str);

void nd_assert_fail(const char *file,
                    nd_uint32_t line,
                    const char *expr,
                    const char *fmt,
                    ...);

#if ND_CFG_DEBUG
    #define ND_ASSERT(expr) \
        do { \
            if (!(expr)) { \
                nd_assert_fail(__FILE__, __LINE__, #expr, ND_NULL); \
            } \
        } while (0)

    #define ND_ASSERT_MSG(expr, fmt, ...) \
        do { \
            if (!(expr)) { \
                nd_assert_fail(__FILE__, __LINE__, #expr, fmt, ##__VA_ARGS__); \
            } \
        } while (0)
#else
    #define ND_ASSERT(expr)               ((void)0)
    #define ND_ASSERT_MSG(expr, fmt, ...) ((void)0)
#endif

#endif /* __ND_ASSERT_H__ */
