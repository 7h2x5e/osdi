#include <include/syscall.h>
#include <include/task.h>
#include <include/types.h>

#define ASM_ARGS_0
#define ASM_ARGS_1 , "r"(_x0)
#define ASM_ARGS_2 ASM_ARGS_1, "r"(_x1)
#define ASM_ARGS_3 ASM_ARGS_2, "r"(_x2)
#define ASM_ARGS_4 ASM_ARGS_3, "r"(_x3)
#define ASM_ARGS_5 ASM_ARGS_4, "r"(_x4)
#define ASM_ARGS_6 ASM_ARGS_5, "r"(_x5)
#define ASM_ARGS_7 ASM_ARGS_6, "r"(_x6)

#define INPUT_ARGS_0
#define INPUT_ARGS_1 x0
#define INPUT_ARGS_2 INPUT_ARGS_1, x1
#define INPUT_ARGS_3 INPUT_ARGS_2, x2
#define INPUT_ARGS_4 INPUT_ARGS_3, x3
#define INPUT_ARGS_5 INPUT_ARGS_4, x4
#define INPUT_ARGS_6 INPUT_ARGS_5, x5
#define INPUT_ARGS_7 INPUT_ARGS_6, x6

#define LOAD_ARGS_0() register uint64_t _x0 asm("x0") __attribute__((unused));
#define LOAD_ARGS_1(x0)                \
    uint64_t _x0tmp = (uint64_t) (x0); \
    LOAD_ARGS_0()                      \
    _x0 = _x0tmp;
#define LOAD_ARGS_2(x0, x1)            \
    uint64_t _x1tmp = (uint64_t) (x1); \
    LOAD_ARGS_1(x0)                    \
    register uint64_t _x1 asm("x1") __attribute__((unused)) = _x1tmp;
#define LOAD_ARGS_3(x0, x1, x2)        \
    uint64_t _x2tmp = (uint64_t) (x2); \
    LOAD_ARGS_2(x0, x1)                \
    register uint64_t _x2 asm("x2") __attribute__((unused)) = _x2tmp;
#define LOAD_ARGS_4(x0, x1, x2, x3)    \
    uint64_t _x3tmp = (uint64_t) (x3); \
    LOAD_ARGS_3(x0, x1, x2)            \
    register uint64_t _x3 asm("x3") __attribute__((unused)) = _x3tmp;
#define LOAD_ARGS_5(x0, x1, x2, x3, x4) \
    uint64_t _x4tmp = (uint64_t) (x4);  \
    LOAD_ARGS_4(x0, x1, x2, x3)         \
    register uint64_t _x4 asm("x4") __attribute__((unused)) = _x4tmp;
#define LOAD_ARGS_6(x0, x1, x2, x3, x4, x5) \
    uint64_t _x5tmp = (uint64_t) (x5);      \
    LOAD_ARGS_5(x0, x1, x2, x3, x4)         \
    register uint64_t _x5 asm("x5") __attribute__((unused)) = _x5tmp;
#define LOAD_ARGS_7(x0, x1, x2, x3, x4, x5, x6) \
    uint64_t _x6tmp = (uint64_t) (x6);          \
    LOAD_ARGS_6(x0, x1, x2, x3, x4, x5)         \
    register uint64_t _x6 asm("x6") __attribute__((unused)) = _x6tmp;

#define INTERNAL_SYSCALL_RAW(name, nr, args...)                  \
    ({                                                           \
        uint64_t _sys_result;                                    \
        {                                                        \
            LOAD_ARGS_##nr(args) register uint64_t _x8 asm("x8") \
                __attribute__((unused)) = (name);                \
            asm volatile("svc   0");                             \
            _sys_result = _x0;                                   \
        }                                                        \
        _sys_result;                                             \
    })

#define SYS_ify(syscall_name) (SYS_##syscall_name)

#define INTERNAL_SYSCALL(name, nr, args...) \
    INTERNAL_SYSCALL_RAW(SYS_ify(name), nr, args)

#define SYSCALL_ARG0(name, ret_t)                               \
    ret_t name()                                                \
    {                                                           \
        return (ret_t) INTERNAL_SYSCALL(name, 0, INPUT_ARGS_0); \
    }
#define SYSCALL_ARG1(name, ret_t, type0)                        \
    ret_t name(type0 x0)                                        \
    {                                                           \
        return (ret_t) INTERNAL_SYSCALL(name, 1, INPUT_ARGS_1); \
    }
#define SYSCALL_ARG2(name, ret_t, type0, type1)                 \
    ret_t name(type0 x0, type1 x1)                              \
    {                                                           \
        return (ret_t) INTERNAL_SYSCALL(name, 2, INPUT_ARGS_2); \
    }
#define SYSCALL_ARG3(name, ret_t, type0, type1, type2)          \
    ret_t name(type0 x0, type1 x1, type2 x2)                    \
    {                                                           \
        return (ret_t) INTERNAL_SYSCALL(name, 3, INPUT_ARGS_3); \
    }
#define SYSCALL_ARG4(name, ret_t, type0, type1, type2, type3)   \
    ret_t name(type0 x0, type1 x1, type2 x2, type3 x3)          \
    {                                                           \
        return (ret_t) INTERNAL_SYSCALL(name, 4, INPUT_ARGS_4); \
    }
#define SYSCALL_ARG5(name, ret_t, type0, type1, type2, type3, type4) \
    ret_t name(type0 x0, type1 x1, type2 x2, type3 x3, type4 x4)     \
    {                                                                \
        return (ret_t) INTERNAL_SYSCALL(name, 5, INPUT_ARGS_5);      \
    }
#define SYSCALL_ARG6(name, ret_t, type0, type1, type2, type3, type4, type5) \
    ret_t name(type0 x0, type1 x1, type2 x2, type3 x3, type4 x4, type5 x5)  \
    {                                                                       \
        return (ret_t) INTERNAL_SYSCALL(name, 6, INPUT_ARGS_6);             \
    }
#define SYSCALL_ARG7(name, ret_t, type0, type1, type2, type3, type4, type5, \
                     type6)                                                 \
    ret_t name(type0 x0, type1 x1, type2 x2, type3 x3, type4 x4, type5 x5,  \
               type6 x6)                                                    \
    {                                                                       \
        return (ret_t) INTERNAL_SYSCALL(name, 7, INPUT_ARGS_7);             \
    }

SYSCALL_ARG1(reset, int64_t, uint64_t)
SYSCALL_ARG0(cancel_reset, int64_t)
SYSCALL_ARG1(get_timestamp, int64_t, struct TimeStamp *)
SYSCALL_ARG2(uart_read, int64_t, void *, size_t)
SYSCALL_ARG2(uart_write, int64_t, void *, size_t)
SYSCALL_ARG0(get_taskid, uint32_t)
SYSCALL_ARG1(exec, int64_t, void *);
SYSCALL_ARG0(fork, int64_t)
SYSCALL_ARG0(exit, int64_t)
SYSCALL_ARG2(kill, int32_t, pid_t, int32_t)
SYSCALL_ARG6(mmap, void *, void *, size_t, int32_t, int32_t, void *, int32_t)
SYSCALL_ARG2(open, int32_t, char *, int32_t)
SYSCALL_ARG1(close, int32_t, int32_t)
SYSCALL_ARG3(read, ssize_t, int32_t, void *, size_t)
SYSCALL_ARG3(write, ssize_t, int32_t, void *, size_t)
SYSCALL_ARG1(mkdir, int32_t, char *)
SYSCALL_ARG1(chdir, int32_t, char *)
SYSCALL_ARG2(getcwd, int32_t, char *, size_t)
SYSCALL_ARG3(mount, int32_t, char *, char *, char *)