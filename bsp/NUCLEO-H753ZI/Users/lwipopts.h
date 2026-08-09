#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/*
 * lwIP 2.2.1 configuration for NerdRTOS on NUCLEO-H753ZI.
 *
 * Style: only options that record an explicit decision are listed here,
 * even when the chosen value equals the opt.h default. Everything absent
 * from this file deliberately uses the opt.h default.
 */

/* ---------- OS mode ---------- */

/* Run with an OS: full sys_arch port, tcpip thread, sequential APIs. */
#define NO_SYS                          0

/* Short critical sections (IRQ save/restore) protect free lists etc. */
#define SYS_LIGHTWEIGHT_PROT            1

/*
 * Core locking model: application threads take one global mutex
 * (LOCK_TCPIP_CORE) and run stack code in their own context, instead of
 * marshalling every API call into the tcpip thread's mailbox.
 */
#define LWIP_TCPIP_CORE_LOCKING         1

/* ---------- Application APIs ---------- */

/* BSD sockets on top of netconn; both enabled. */
#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     1

/* ---------- Protocols ---------- */

#define LWIP_IPV6                       0

/* Bring-up uses a static IP; enable DHCP only after ping works. */
#define LWIP_DHCP                       0

/* ---------- Memory ---------- */

/* Cortex-M7, 32-bit aligned accesses. */
#define MEM_ALIGNMENT                   4

/*
 * Stack-private static heap (mem_malloc), isolated from nd_mem.
 * The opt.h default (1600 B) is far too small for real TCP traffic.
 * Tune with MEM_STATS once running.
 */
#define MEM_SIZE                        (32 * 1024)

/*
 * RX buffers are owned by the STM32 driver and wrapped in custom pbufs.
 * Their free callback returns each SRAM3 buffer to the ETH DMA.
 */
#define LWIP_SUPPORT_CUSTOM_PBUF         1

/* ---------- tcpip thread & mailboxes ---------- */
/* opt.h defaults all of these to 0, which is unusable; a port MUST set them. */

#define TCPIP_THREAD_NAME               "tcpip"
#define TCPIP_THREAD_STACKSIZE          4096    /* bytes; trim later with nd_thread_stack_used() */
#define TCPIP_THREAD_PRIO               9       /* NerdRTOS: lower number = higher priority; shell runs at 30 */

#define DEFAULT_THREAD_STACKSIZE        2048
#define DEFAULT_THREAD_PRIO             20

#define TCPIP_MBOX_SIZE                 16
#define DEFAULT_RAW_RECVMBOX_SIZE       8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_TCP_RECVMBOX_SIZE       16
#define DEFAULT_ACCEPTMBOX_SIZE         4

/* ---------- TCP throughput ---------- */

/* Ethernet MTU 1500 minus the IPv4 and TCP headers. */
#define TCP_MSS                         1460
#define TCP_WND                         (8 * TCP_MSS)
#define TCP_SND_BUF                     (8 * TCP_MSS)
#define MEMP_NUM_TCP_SEG                32

/* ---------- STM32H7 Ethernet checksum offload ---------- */

/* ICMP remains software-generated; IPv4/TCP/UDP use the MAC checksum unit. */
#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0

#endif /* LWIPOPTS_H */
