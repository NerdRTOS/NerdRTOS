#include "network_perf.h"

#include "lwip/def.h"
#include "lwip/sockets.h"
#include "nd_klibc.h"
#include "nd_shell.h"
#include "nerd.h"

#define NETWORK_PERF_PORT               (5001U)
#define NETWORK_PERF_BACKLOG            (1)
#define NETWORK_PERF_THREAD_STACK_SIZE  (2048U)
#define NETWORK_PERF_THREAD_PRIORITY    (11U)
#define NETWORK_PERF_BUFFER_SIZE        (1460U)

#define NETWORK_PERF_RECEIVE_COMMAND    ('R')
#define NETWORK_PERF_TRANSMIT_COMMAND   ('T')

static nd_thread_t network_perf_thread;
static nd_uint8_t network_perf_thread_stack[NETWORK_PERF_THREAD_STACK_SIZE];
static nd_uint8_t network_perf_buffer[NETWORK_PERF_BUFFER_SIZE];

static void network_perf_receive(int socket)
{
    ssize_t received;
    nd_uint32_t total = 0U;

    while ((received = lwip_recv(socket, network_perf_buffer,
                                 sizeof(network_perf_buffer), 0)) > 0) {
        total += (nd_uint32_t)received;
    }

    shell_printf("[netperf] TCP RX: %u bytes\r\n", total);
}

static void network_perf_transmit(int socket)
{
    ssize_t sent;
    nd_uint32_t total = 0U;

    for (;;) {
        sent = lwip_send(socket, network_perf_buffer,
                         sizeof(network_perf_buffer), 0);
        if (sent <= 0) {
            break;
        }
        total += (nd_uint32_t)sent;
    }

    shell_printf("[netperf] TCP TX: %u bytes\r\n", total);
}

static void network_perf_thread_entry(void *parameter)
{
    struct sockaddr_in address;
    char command;
    int listener;
    int socket;

    (void)parameter;

    listener = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        shell_printf("[netperf] socket creation failed\r\n");
        return;
    }

    nd_memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = PP_HTONS(NETWORK_PERF_PORT);
    address.sin_addr.s_addr = PP_HTONL(INADDR_ANY);

    if (lwip_bind(listener, (const struct sockaddr *)&address,
                  sizeof(address)) != 0 ||
        lwip_listen(listener, NETWORK_PERF_BACKLOG) != 0) {
        shell_printf("[netperf] bind/listen failed\r\n");
        (void)lwip_close(listener);
        return;
    }

    shell_printf("[netperf] TCP server listening on port %u\r\n",
                 NETWORK_PERF_PORT);

    for (;;) {
        socket = lwip_accept(listener, NULL, NULL);
        if (socket < 0) {
            continue;
        }

        if (lwip_recv(socket, &command, sizeof(command), 0) ==
            (ssize_t)sizeof(command)) {
            if (command == NETWORK_PERF_RECEIVE_COMMAND) {
                network_perf_receive(socket);
            } else if (command == NETWORK_PERF_TRANSMIT_COMMAND) {
                network_perf_transmit(socket);
            }
        }

        (void)lwip_close(socket);
    }
}

nd_err_t network_perf_init(void)
{
    nd_memset(network_perf_buffer, 0xA5, sizeof(network_perf_buffer));

    return nd_thread_create(&network_perf_thread, "netperf",
                            network_perf_thread_entry,
                            NETWORK_PERF_THREAD_PRIORITY, NULL,
                            network_perf_thread_stack,
                            sizeof(network_perf_thread_stack),
                            ND_THREAD_OPT_NONE, 0U);
}
