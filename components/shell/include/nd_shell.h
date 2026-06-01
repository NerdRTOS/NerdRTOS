#ifndef __ND_SHELL_H__
#define __ND_SHELL_H__

#include "nd_def.h"

typedef void (*shell_func_t)(int argc, char *argv[]);
struct nd_shell_cmd{
    const char *name;
    const char *desc;
    shell_func_t func;
};

typedef enum {
    SHELL_INPUT_NORMAL = 0,
    SHELL_INPUT_ESC,
    SHELL_INPUT_CSI,
} shell_input_state_t;

#define SHELL_CMD(name, func, desc) \
    const struct nd_shell_cmd _shell_cmd_##name \
    __attribute__((section(".shell_cmd"), used, aligned(4))) = { #name, desc, func }

void shell_init(void);
char shell_getc(void);
nd_int32_t shell_putc(char c);
void shell_puts(const char *str);
void shell_printf(const char *fmt, ...);
void nd_shell_task_entry(void *parameter);

#endif
