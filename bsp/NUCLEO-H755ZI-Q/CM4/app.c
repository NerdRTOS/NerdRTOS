#include "nd_shell.h"
#include "app.h"

#define SHELL_STACK_SIZE    1024

static nd_uint8_t   shell_stack[SHELL_STACK_SIZE];
static nd_thread_t  shell;

void nd_app_init(void)
{
    shell_init();

    nd_thread_create(&shell, "shell", nd_shell_task_entry,
                     30, ND_NULL, shell_stack, sizeof(shell_stack), ND_THREAD_OPT_NONE, 0);
}
