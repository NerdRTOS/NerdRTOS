#ifndef __ND_DEF_H__
#define __ND_DEF_H__

typedef signed   char                   nd_int8_t;
typedef signed   short                  nd_int16_t;
typedef signed   int                    nd_int32_t;
typedef unsigned char                   nd_uint8_t;
typedef unsigned short                  nd_uint16_t;
typedef unsigned int                    nd_uint32_t;

#ifndef ARCH_CPU_64BIT
typedef long long                       nd_int64_t;
typedef unsigned long long              nd_uint64_t;
#else
typedef long                            nd_int64_t;
typedef unsigned long                   nd_uint64_t;
#endif

typedef int                             nd_bool_t;

typedef long                            nd_base_t;
typedef unsigned long                   nd_ubase_t;
typedef nd_ubase_t                      nd_size_t;

#define ND_TRUE                         1
#define ND_FALSE                        0

typedef unsigned long                   nd_err_t;        // Type for error number

#define ND_EOK                          0               // There is no error
#define ND_ERROR                        1               // A generic error happens
#define ND_ETIMEOUT                     2               // Timed out
#define ND_EFULL                        3               // The resource is full
#define ND_EEMPTY                       4               // The resource is empty
#define ND_ENOMEM                       5               // No memory
#define ND_ENOSYS                       6               // No system
#define ND_EBUSY                        7               // Busy
#define ND_EIO                          8               // IO error
#define ND_EINTR                        9               // Interrupted system call
#define ND_EINVAL                      10               // Invalid argument
#define ND_EPERM                       11               // Permission denied
#define ND_EDEADLK                     12               // Deadlock

#ifdef __CC_ARM
        #define nd_inline                   static __inline
        #define ALIGN(n)                    __attribute__((aligned(n)))

#elif defined (__IAR_SYSTEMS_ICC__)
    #define nd_inline                   static inline
        #define ALIGN(n)                    PRAGMA(data_alignment=n)

#elif defined (__GNUC__)
    #define nd_inline                   static __inline
        #define ALIGN(n)                    __attribute__((aligned(n)))
#else
    #error not supported tool chain
#endif

#define ND_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))
#define ND_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

#define ND_TIMEOUT_FOREVER          ((nd_uint64_t) -1)
#define ND_TIMEOUT_NOWAIT           ((nd_uint64_t) 0)

#define ND_NULL                         (0)

#define ND_CONFIG_YES                   (1)
#define ND_CONFIG_NO                    (0)

#define ND_YES                          ND_CONFIG_YES
#define ND_NO                           ND_CONFIG_NO

#if ND_CFG_DEBUG
    #define ND_ASSERT(expr)  do { if (!(expr)) { while (1); } } while (0)
#else
    #define ND_ASSERT(expr)  ((void)0)
#endif

#endif /* __ND_DEF_H__*/
