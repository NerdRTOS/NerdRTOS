#include <stdarg.h>

#include "nd_assert.h"
#include "nd_klibc.h"

void nd_assert_fail(const char *file,
                    nd_uint32_t line,
                    const char *expr,
                    const char *fmt,
                    ...)
{
    char buf[ND_ASSERT_BUF_SIZE];
    va_list args;

    nd_snprintf(buf, sizeof(buf),
                "\r\nASSERTION FAIL [%s] @ %s:%u\r\n",
                expr, file, line);
    nd_assert_puts(buf);

    if (fmt != ND_NULL) {
        va_start(args, fmt);
        nd_vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        nd_assert_puts("    ");
        nd_assert_puts(buf);
        nd_assert_puts("\r\n");
    }

    while (1) {
    }
}
