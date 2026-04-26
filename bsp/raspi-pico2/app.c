#include "app.h"
#include <string.h>

#define SHELL_STACK_SIZE        2048

static nd_thread_t shell;
static nd_uint8_t shell_stack[SHELL_STACK_SIZE];

void nd_app_init(void)
{
    shell_init();

    nd_thread_create(&shell, "shell", nd_shell_task_entry,
                 30, ND_NULL, shell_stack, sizeof(shell_stack), 0);
}
