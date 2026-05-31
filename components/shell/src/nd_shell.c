#include "nd_shell.h"
#include "nd_def.h"
#include "nd_klibc.h"

#define SHELL_PROMPT            "nerd@rtos:~$ "
#define SHELL_BUFFER_SIZE       128
#define ARGC_MAX                8

#define ASCII_PRINTABLE_START   0x20
#define ASCII_PRINTABLE_END     0x7E
#define ASCII_CR                '\r'
#define ASCII_LF                '\n'
#define ASCII_BACKSPACE         0x08
#define ASCII_DEL               0x7F
#define ASCII_NULL              '\0'

extern const struct nd_shell_cmd __shell_cmd_start;
extern const struct nd_shell_cmd __shell_cmd_end;

static char shell_buf[SHELL_BUFFER_SIZE];
static nd_uint8_t shell_len = 0;

static char shell_last_line_end = ASCII_NULL;

static nd_bool_t shell_is_line_end(char c)
{
    if (c != ASCII_CR && c != ASCII_LF) {
        shell_last_line_end = ASCII_NULL;
        return ND_FALSE;
    }

    if (shell_last_line_end == ASCII_NULL || c == shell_last_line_end) {
        shell_last_line_end = c;
        return ND_TRUE;
    }

    return ND_FALSE;
}

static nd_bool_t shell_input(char c)
{
    if (shell_is_line_end(c)) {
        shell_puts("\r\n");
        shell_buf[shell_len] = '\0';
        return ND_TRUE;
    }

    if (c == ASCII_BACKSPACE || c == ASCII_DEL) {
        if (shell_len > 0) {
            shell_len--;
            shell_putc('\b');
            shell_putc(' ');
            shell_putc('\b');
        }
        return ND_FALSE;
    }

    if (c >= ASCII_PRINTABLE_START && c <= ASCII_PRINTABLE_END) {
        if (shell_len < SHELL_BUFFER_SIZE - 1) {
            shell_buf[shell_len++] = c;
            shell_putc(c);
        }
    }

    return ND_FALSE;
}

static nd_uint8_t shell_parse(char *cmd_line, char **argv, nd_uint8_t max_argc)
{
    nd_uint8_t argc = 0;
    char *save_ptr = ND_NULL;

    char *token = nd_strtok_r(cmd_line, " ", &save_ptr);
    while (token && argc < max_argc) {
        argv[argc++] = token;
        token = nd_strtok_r(ND_NULL, " ", &save_ptr);
    }

    return argc;
}

static const struct nd_shell_cmd *shell_find(const char *name)
{
    const struct nd_shell_cmd *cmd;

    for (cmd = &__shell_cmd_start; cmd < &__shell_cmd_end; cmd++) {
        if (cmd->name && nd_strcmp(name, cmd->name) == 0) {
            return cmd;
        }
    }

    return ND_NULL;
}

void nd_shell_task_entry(void *para)
{
    (void)para;

    char *argv[ARGC_MAX];

    shell_puts(SHELL_PROMPT);

    for (;;) {
        char c = shell_getc();

        if (!shell_input(c)) {
            continue;
        }

        if (shell_len > 0) {
            nd_uint8_t argc = shell_parse(shell_buf, argv, ARGC_MAX);

            if (argc > 0) {
                const struct nd_shell_cmd *cmd = shell_find(argv[0]);
                if (cmd)
                    cmd->func(argc,argv);
                else
                    shell_puts("command not found\r\n");
            }
        }
        shell_len = 0;
        shell_puts(SHELL_PROMPT);
    }
}

static void nd_shell_help(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    const struct nd_shell_cmd *cmd;

    shell_puts("\r\n");

    for (cmd = &__shell_cmd_start; cmd < &__shell_cmd_end; cmd++) {
        if (!cmd->name || !cmd->desc) {
            continue;
        }

        shell_puts(" - ");
        shell_puts(cmd->name);
        shell_puts(" - ");
        shell_puts(cmd->desc);
        shell_puts("\r\n");
    }
}

SHELL_CMD(help, nd_shell_help, "list command");
