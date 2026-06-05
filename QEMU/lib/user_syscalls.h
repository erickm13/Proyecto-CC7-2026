#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

#include <stdint.h>
#include <stddef.h>

enum {
    SYS_YIELD = 0,
    SYS_EXIT  = 1,
    SYS_WRITE = 2,
};

static inline int32_t syscall3(uint32_t id, uint32_t a1, uint32_t a2, uint32_t a3) {
    register uint32_t r0 asm("r0") = id;
    register uint32_t r1 asm("r1") = a1;
    register uint32_t r2 asm("r2") = a2;
    register uint32_t r3 asm("r3") = a3;

    asm volatile(
        "svc #0\n"
        : "+r"(r0)
        : "r"(r1), "r"(r2), "r"(r3)
        : "memory"
    );

    return (int32_t)r0;
}

static inline int32_t sys_yield(void) {
    return syscall3(SYS_YIELD, 0, 0, 0);
}

static inline void sys_exit(int32_t code) {
    (void)syscall3(SYS_EXIT, (uint32_t)code, 0, 0);
    while (1) { }
}

static inline int32_t sys_write(int32_t fd, const void *buf, size_t len) {
    return syscall3(SYS_WRITE, (uint32_t)fd, (uint32_t)buf, (uint32_t)len);
}

#endif
