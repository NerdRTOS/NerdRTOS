#ifndef __NTEST_H__
#define __NTEST_H__

#include "nd_def.h"
#include "nd_shell.h"

typedef struct {
    const char  *suite;
    const char  *current;
    nd_bool_t   current_failed;
    nd_uint32_t passed;
    nd_uint32_t failed;
} ntest_ctx_t;

extern ntest_ctx_t __ntest_ctx;

#define ntest_assert_true(expr) do {                              \
    if (!(expr)) {                                                \
        shell_printf(" FAIL %s:%d\r\n",                           \
                     __ntest_ctx.current, __LINE__);              \
        __ntest_ctx.current_failed = ND_TRUE;                     \
        return;                                                   \
    }                                                             \
} while (0)

#define ntest_assert_false(expr)       ntest_assert_true(!(expr))
#define ntest_assert_equal(a, b)       ntest_assert_true((a) == (b))
#define ntest_assert_not_equal(a, b)   ntest_assert_true((a) != (b))
#define ntest_assert_null(p)           ntest_assert_true((p) == ND_NULL)
#define ntest_assert_not_null(p)       ntest_assert_true((p) != ND_NULL)

#define NTEST_SUITE_BEGIN(name) do {                              \
    __ntest_ctx.passed = 0;                                       \
    __ntest_ctx.failed = 0;                                       \
    __ntest_ctx.suite = (name);                                   \
    shell_printf("--- %s ---\r\n", name);                         \
} while (0)

#define NTEST_RUN(fn) do {                                        \
    __ntest_ctx.current = #fn;                                    \
    __ntest_ctx.current_failed = ND_FALSE;                        \
    shell_printf("START %s\r\n", #fn);                            \
    fn();                                                         \
    if (__ntest_ctx.current_failed) {                             \
        __ntest_ctx.failed++;                                     \
    } else {                                                      \
        __ntest_ctx.passed++;                                     \
        shell_printf(" PASS %s\r\n", #fn);                        \
    }                                                             \
} while (0)

#define NTEST_SUITE_END() do {                                    \
    shell_printf("--- %s: %d passed, %d failed ---\r\n",          \
                 __ntest_ctx.suite,                               \
                 __ntest_ctx.passed,                              \
                 __ntest_ctx.failed);                             \
} while (0)

#endif /* __NTEST_H__ */
