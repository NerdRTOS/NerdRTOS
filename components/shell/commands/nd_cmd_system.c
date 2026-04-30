#include "nd_shell.h"
<<<<<<< HEAD
=======
#include "nd_cpuload.h"
#include "nd_internal.h"
>>>>>>> 6f17a0a4d2c281b6632fa60cd4cad536ad20e7f9
#include "nerd.h"

static void cmd_ps(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    nd_list_t *list_head = nd_thread_list_get();
    nd_thread_t *thread;

    nd_uint32_t cpu_load = nd_cpuload_get();

    static const char *state_strs[] = {
        [ND_THREAD_STAT_INIT]    = "INIT",
        [ND_THREAD_STAT_READY]   = "READY",
        [ND_THREAD_STAT_RUNNING] = "RUN",
        [ND_THREAD_STAT_BLOCK]   = "BLOCK",
        [ND_THREAD_STAT_DEAD]    = "DEAD",
        [ND_THREAD_STAT_SUSPEND] = "SUSPEND",
    };

    shell_printf("\r\n");
<<<<<<< HEAD
    shell_printf("%-10s %-8s %5s %7s %7s\r\n", "Name", "State", "Pri", "Stack", "Used");
    shell_printf("---------- -------- ----- ------- -------\r\n");

    for (node = list_head->next; node != list_head; node = node->next) {
        nd_thread_t *thread = nd_list_entry(node, nd_thread_t, tlist);
=======
    shell_printf("CPU Load: %u%%\r\n", cpu_load);
    shell_printf("%-10s %-8s %5s %7s %7s %6s\r\n", "Name", "State", "Pri", "Stack", "Used", "Load");
    shell_printf("---------- -------- ----- ------- ------- ------\r\n");
>>>>>>> 6f17a0a4d2c281b6632fa60cd4cad536ad20e7f9

    nd_list_for_each_entry(thread, list_head, tlist) {
        nd_uint32_t used = nd_thread_stack_used(thread);
        const char *stat_str = (thread->stat < sizeof(state_strs) / sizeof(state_strs[0])) ?
                               state_strs[thread->stat] : "ERR";

<<<<<<< HEAD
        const char *stat_str = (thread->stat < sizeof(state_strs)/sizeof(state_strs[0])) ?
                                state_strs[thread->stat] : "ERR";

        shell_printf("%-10s %-8s %5u %7u %7u\r\n",
=======
        shell_printf("%-10s %-8s %5u %7u %7u %5u%%\r\n",
>>>>>>> 6f17a0a4d2c281b6632fa60cd4cad536ad20e7f9
               thread->name,
               stat_str,
               thread->priority,
               thread->stack_size,
               used,
               nd_cpuload_thread_get(thread));
    }
}

SHELL_CMD(ps, cmd_ps, "Thread information");

static void cmd_load(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    shell_printf("CPU load : %u%%\r\n", nd_cpuload_get());
}

SHELL_CMD(load, cmd_load, "CPU load");
