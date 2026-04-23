#include "nd_def.h"
#include "nd_klibc.h"

static void buf_putc(char *buf, size_t size, nd_uint32_t *written, char c)
{
    if (*written < size - 1) {
        buf[(*written)++] = c;
    }
}

static void buf_putn(char *buf, size_t size, nd_uint32_t *written, const char *str, nd_uint32_t len)
{
    while (len--) {
        buf_putc(buf, size, written, *str++);
    }
}

static void buf_pad(char *buf, size_t size, nd_uint32_t *written, nd_uint32_t count)
{
    while (count--) {
        buf_putc(buf, size, written, ' ');
    }
}

static int is_delim(char c, const char *delimiters) {
    while (*delimiters) {
        if (c == *delimiters) {
            return 1;
        }
        delimiters++;
    }
    return 0;
}

char *nd_strtok_r(char *str, const char *delim, char **saveptr) {
    char *token_start;

    if (str == ND_NULL) {
        str = *saveptr;
    }

    while (*str && is_delim(*str, delim)) {
        str++;
    }

    if (*str == '\0') {
        *saveptr = str;
        return ND_NULL;
    }

    token_start = str;

    while (*str && !is_delim(*str, delim)) {
        str++;
    }

    if (*str == '\0') {
        *saveptr = str;
    } else {
        *str = '\0';
        *saveptr = str + 1;
    }

    return token_start;
}

int nd_vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    nd_uint32_t written = 0;
    nd_uint32_t len = 0;
    char tmp_buf[24];

    if (size == 0) return 0;

    while (*fmt && written < size - 1) {
        if (*fmt != '%') {
            buf[written++] = *fmt++;
            continue;
        }

        fmt++;

        int left_align = 0;
        nd_uint32_t width = 0;
        int is_long = 0;

        if (*fmt == '-') {
            left_align = 1;
            fmt++;
        }

        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (nd_uint32_t)(*fmt - '0');
            fmt++;
        }

        if (*fmt == 'l') {
            is_long = 1;
            fmt++;
        }

        const char *out = tmp_buf;

        switch (*fmt) {
        case 'd': {
            long num;
            unsigned long value;

            if (is_long)
                num = va_arg(args, long);
            else
                num = va_arg(args, int);

            len = 0;

            if (num < 0) {
                tmp_buf[len++] = '-';
                value = (unsigned long)(-num);
            } else {
                value = (unsigned long)num;
            }

            if (value == 0) {
                tmp_buf[len++] = '0';
                break;
            }

            nd_uint32_t start = len;
            while (value > 0) {
                tmp_buf[len++] = value % 10 + '0';
                value /= 10;
            }

            for (nd_uint32_t i = start, j = len - 1; i < j; i++, j--) {
                char tmp = tmp_buf[i];
                tmp_buf[i] = tmp_buf[j];
                tmp_buf[j] = tmp;
            }
            break;
        }
        case 'u': {
            unsigned long num;

            if (is_long)
                num = va_arg(args, unsigned long);
            else
                num = va_arg(args, unsigned int);

            len = 0;
            if (num == 0) {
                tmp_buf[len++] = '0';
                break;
            }

            while (num > 0) {
                tmp_buf[len++] = num % 10 + '0';
                num /= 10;
            }

            for (nd_uint32_t i = 0, j = len - 1; i < j; i++, j--) {
                char tmp = tmp_buf[i];
                tmp_buf[i] = tmp_buf[j];
                tmp_buf[j] = tmp;
            }
            break;
        }
        case 'x': {
            unsigned long num;

            if (is_long)
                num = va_arg(args, unsigned long);
            else
                num = va_arg(args, unsigned int);

            len = 0;
            if (num == 0) {
                tmp_buf[len++] = '0';
                break;
            }

            while (num > 0) {
                nd_uint8_t digit = num & 0xF;
                tmp_buf[len++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
                num >>= 4;
            }

            for (nd_uint32_t i = 0, j = len - 1; i < j; i++, j--) {
                char tmp = tmp_buf[i];
                tmp_buf[i] = tmp_buf[j];
                tmp_buf[j] = tmp;
            }
            break;
        }
        case 's': {
            const char *s = va_arg(args, const char *);
            out = s ? s : "(null)";
            len = nd_strlen(out);
            break;
        }
        case 'c':
            tmp_buf[0] = (char)va_arg(args, int);
            len = 1;
            break;
        case '%':
            tmp_buf[0] = '%';
            len = 1;
            break;
        default:
            tmp_buf[0] = '%';
            tmp_buf[1] = *fmt;
            len = 2;
            break;
        }

        if (!left_align && width > len) {
            buf_pad(buf, size, &written, width - len);
        }

        buf_putn(buf, size, &written, out, len);

        if (left_align && width > len) {
            buf_pad(buf, size, &written, width - len);
        }

        fmt++;
    }

    buf[written] = '\0';

    return written;
}

int nd_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    int ret = nd_vsnprintf(buf, size, fmt, args);

    va_end(args);

    return ret;
}

char *nd_strncpy(char *dest, const char *src, size_t count)
{
    char *ret = dest;
    while (count && (*src != '\0')) {
        *dest++ = *src++;
        count--;
    }
    while (count) {
        *dest++ = '\0';
        count--;
    }
    return ret;
}

nd_size_t nd_strlen(const char *s)
{
    const char *sc;

    for (sc = s; *sc != '\0'; ++sc)
        /* do nothing */;

    return sc - s;
}

void *nd_memcpy(void *dest, const void *src, size_t count)
{
    char *tmp = dest;
    const char *s = src;

    while (count--) {
        *tmp++ = *s++;
    }

    return dest;
}

nd_int32_t nd_strcmp(const char *cs, const char *ct)
{
    while (*cs && *cs == *ct) {
        cs++;
        ct++;
    }

    return (*(unsigned char *)cs - *(unsigned char *)ct);
}

void *nd_memset(void *s, int c, size_t count)
{
    char *tmp = s;

    while (count--) {
        *tmp++ = c;
    }

    return s;
}
