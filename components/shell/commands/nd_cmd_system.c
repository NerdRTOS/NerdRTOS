#include <stdio.h>
#include "nd_shell.h"
#include "nerd.h"

static void cmd_ps(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    nd_list_t *list_head = nd_thread_list_get();
    nd_list_t *node;

    static const char *state_strs[] = {
        [ND_THREAD_STAT_INIT]    = "INIT",
        [ND_THREAD_STAT_READY]   = "READY",
        [ND_THREAD_STAT_RUNNING] = "RUN",
        [ND_THREAD_STAT_BLOCK]   = "BLOCK",
        [ND_THREAD_STAT_END]     = "END",
        [ND_THREAD_STAT_SUSPEND] = "SUSPEND",
    };

    printf("\r\n");
    printf("%-10s %-8s %5s %7s %7s\r\n", "Name", "State", "Pri", "Stack", "Used");
    printf("---------- -------- ----- ------- -------\r\n");

    for (node = list_head->next; node != list_head; node = node->next) {
        nd_thread_t *thread = nd_list_entry(node, nd_thread_t, tlist);

        nd_uint32_t used = nd_thread_stack_used(thread);

        const char *stat_str = (thread->stat < sizeof(state_strs)/sizeof(state_strs[0])) ?
                                state_strs[thread->stat] : "ERR";

        printf("%-10s %-8s %5d %7d %7d\r\n",
               thread->name,
               stat_str,
               thread->priority,
               thread->stack_size,
               used);
    }
}

SHELL_CMD(ps, cmd_ps, "Thread information");
