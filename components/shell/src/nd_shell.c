#include "nd_shell.h"
#include "nd_def.h"
#include "nd_klibc.h"

#define SHELL_PROMPT            "nerd@rtos:~$ "
#define SHELL_BUFFER_SIZE       128
#define SHELL_HISTORY_DEPTH     8
#define ARGC_MAX                8

#define ASCII_PRINTABLE_START   0x20
#define ASCII_PRINTABLE_END     0x7E
#define ASCII_CR                '\r'
#define ASCII_LF                '\n'
#define ASCII_BACKSPACE         0x08
#define ASCII_DEL               0x7F
#define ASCII_NULL              '\0'
#define ASCII_ESC               0x1B

#define ANSI_CSI_PREFIX         '['
#define ANSI_ARROW_UP           'A'
#define ANSI_ARROW_DOWN         'B'

extern const struct nd_shell_cmd __shell_cmd_start;
extern const struct nd_shell_cmd __shell_cmd_end;

static char shell_buf[SHELL_BUFFER_SIZE];
static nd_uint8_t shell_len = 0;
static char shell_last_line_end = ASCII_NULL;

typedef struct {
    char history[SHELL_HISTORY_DEPTH][SHELL_BUFFER_SIZE];
    nd_uint8_t count;
    nd_uint8_t write_pos;
    nd_int8_t browse_pos;
    shell_input_state_t state;
} shell_history_t;

static shell_history_t shell_history;

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

static void shell_replace_input(const char *line)
{
    while (shell_len > 0) {
        shell_len--;
        shell_putc('\b');
        shell_putc(' ');
        shell_putc('\b');
    }

    while (line[shell_len] != ASCII_NULL && shell_len < SHELL_BUFFER_SIZE - 1) {
        shell_buf[shell_len] = line[shell_len];
        shell_putc(line[shell_len]);
        shell_len++;
    }

    shell_buf[shell_len] = ASCII_NULL;
}

static nd_uint8_t shell_history_browse_index(void)
{
    return (shell_history.write_pos + SHELL_HISTORY_DEPTH - 1 - shell_history.browse_pos) %
           SHELL_HISTORY_DEPTH;
}

static void shell_history_prev(void)
{
    if (shell_history.count == 0 ||
        shell_history.browse_pos >= (nd_int8_t)(shell_history.count - 1)) {
        return;
    }

    shell_history.browse_pos++;
    shell_replace_input(shell_history.history[shell_history_browse_index()]);
}

static void shell_history_next(void)
{
    if (shell_history.browse_pos < 0) {
        return;
    }

    shell_history.browse_pos--;

    if (shell_history.browse_pos < 0) {
        shell_replace_input("");
        return;
    }

    shell_replace_input(shell_history.history[shell_history_browse_index()]);
}

static nd_bool_t shell_handle_ansi(char c) {
    switch (shell_history.state) {
    case SHELL_INPUT_NORMAL:
        if (c == ASCII_ESC) {
            shell_history.state = SHELL_INPUT_ESC;
            return ND_TRUE;
        }
        return ND_FALSE;

    case SHELL_INPUT_ESC:
        if (c == ANSI_CSI_PREFIX) {
            shell_history.state = SHELL_INPUT_CSI;
        } else {
            shell_history.state = SHELL_INPUT_NORMAL;
        }
        return ND_TRUE;

    case SHELL_INPUT_CSI:
        if (c == ANSI_ARROW_UP) {
            shell_history_prev();
        } else if (c == ANSI_ARROW_DOWN) {
            shell_history_next();
        }
        shell_history.state = SHELL_INPUT_NORMAL;
        return ND_TRUE;
    }

    return ND_FALSE;
}

static nd_bool_t shell_input(char c)
{
    if (shell_handle_ansi(c)) {
        return ND_FALSE;
    }

    if (shell_is_line_end(c)) {
        shell_puts("\r\n");
        shell_buf[shell_len] = '\0';
        shell_history.browse_pos = -1;
        return ND_TRUE;
    }

    if (c == ASCII_BACKSPACE || c == ASCII_DEL) {
        if (shell_len > 0) {
            shell_len--;
            shell_putc('\b');
            shell_putc(' ');
            shell_putc('\b');
            shell_history.browse_pos = -1;
        }
        return ND_FALSE;
    }

    if (c >= ASCII_PRINTABLE_START && c <= ASCII_PRINTABLE_END) {
        if (shell_len < SHELL_BUFFER_SIZE - 1) {
            shell_history.browse_pos = -1;
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

static void shell_history_init(void)
{
    shell_history.state = SHELL_INPUT_NORMAL;
    shell_history.count = 0;
    shell_history.write_pos = 0;
    shell_history.browse_pos = -1;
}

static nd_bool_t shell_history_is_last_duplicate(const char *line)
{
    if (shell_history.count == 0) {
        return ND_FALSE;
    }

    return nd_strcmp(shell_history.history[(shell_history.write_pos + SHELL_HISTORY_DEPTH - 1) % SHELL_HISTORY_DEPTH], line) == 0;
}

static void shell_history_save(char *buf)
{
    if (shell_history_is_last_duplicate(buf)) {
        return;
    }

    nd_strncpy(shell_history.history[shell_history.write_pos], buf, SHELL_BUFFER_SIZE);

    if (shell_history.count < SHELL_HISTORY_DEPTH) {
        shell_history.count++;
    }

    shell_history.write_pos = (shell_history.write_pos + 1) % SHELL_HISTORY_DEPTH;
}

void nd_shell_task_entry(void *para)
{
    (void)para;

    char *argv[ARGC_MAX];

    shell_history_init();

    shell_puts(SHELL_PROMPT);

    for (;;) {
        char c = shell_getc();

        if (!shell_input(c)) {
            continue;
        }

        if (shell_len > 0) {
            shell_history_save(shell_buf);

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
