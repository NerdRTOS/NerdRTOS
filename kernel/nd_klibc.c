#include "nd_def.h"
#include "nd_klibc.h"

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

    char tmp_buf[12];

    if (size == 0) return 0;

    while (*fmt && written < size - 1) {
        if (*fmt != '%') {
            buf[written++] = *fmt++;
            continue;
        }

        fmt++;

        /* 解析可选的 'l' 修饰符 */
        int is_long = 0;
        if (*fmt == 'l') {
            is_long = 1;
            fmt++;
        }

        switch (*fmt) {
        case 'd': {
            long num;
            if (is_long)
                num = va_arg(args, long);
            else
                num = va_arg(args, int);
            len = 0;
            if (num == 0) {
                buf[written++] = '0';
                break;
            }
            if (num < 0) {
                buf[written++] = '-';
                num = -num;
            }
            while (num > 0) {
                tmp_buf[len++] = num % 10 + '0';
                num /= 10;
            }
            while (len > 0 && written < size - 1) {
                buf[written++] = tmp_buf[--len];
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
                buf[written++] = '0';
                break;
            }
            while (num > 0) {
                tmp_buf[len++] = num % 10 + '0';
                num /= 10;
            }
            while (len > 0 && written < size - 1) {
                buf[written++] = tmp_buf[--len];
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
                buf[written++] = '0';
                break;
            }
            while (num > 0) {
                nd_uint8_t digit = num & 0xF;
                tmp_buf[len++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
                num >>= 4;
            }
            while (len > 0 && written < size - 1) {
                buf[written++] = tmp_buf[--len];
            }
            break;
        }
        case 's': {
            const char *s = va_arg(args, const char *);
            if (!s) s = "(null)";
            while (*s && written < size - 1) {
                buf[written++] = *s++;
            }
            break;
        }
        case 'c':
            buf[written++] = (char)va_arg(args, int);
            break;
        case '%':
            buf[written++] = '%';
            break;
        default:
            buf[written++] = '%';
            if (written < size - 1)
                buf[written++] = *fmt;
            break;
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
