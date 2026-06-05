#include "os.h"
#include "../lib/uart.h"
#include "../lib/string.h"
#include "timer.h"

// Global PCB array for all processes
PCB pcb[MAX_PROCESSES];

// Index of currently running process
uint32_t current_process = 0;
static uint32_t rr_ticks_remaining = RR_QUANTUM_TICKS;

enum {
    SYS_YIELD = 0,
    SYS_EXIT = 1,
    SYS_WRITE = 2,
};

enum {
    TF_SPSR = 0,
    TF_R0 = 1,
    TF_R1 = 2,
    TF_R2 = 3,
    TF_R3 = 4,
    TF_PC = 14,
};

static void os_write_uint(uint32_t value) {
    char buf[10];
    int i = 0;

    if (value == 0) {
        uart_putc('0');
        return;
    }

    while (value > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

static void os_write_int(int32_t value) {
    if (value < 0) {
        uart_putc('-');
        os_write_uint((uint32_t)(-value));
        return;
    }
    os_write_uint((uint32_t)value);
}

static void trace_user_to_kernel(const char *reason) {
    os_write("MODE_SWITCH USER_TO_KERNEL pid=");
    os_write_uint(current_process);
    os_write(" reason=");
    os_write(reason);
}

static void trace_kernel_to_user(uint32_t pid, const char *reason) {
    os_write("MODE_SWITCH KERNEL_TO_USER pid=");
    os_write_uint(pid);
    os_write(" reason=");
    os_write(reason);
}

static int is_runnable(uint32_t pid) {
    return pcb[pid].state == READY || pcb[pid].state == RUNNING;
}

static int valid_user_range(uint32_t pid, uint32_t ptr, uint32_t len) {
    if (len == 0) {
        return 1;
    }
    if (ptr + len < ptr) {
        return 0;
    }

    if (pid < FIRST_USER_PID || pid >= MAX_PROCESSES) {
        return 0;
    }

    return ptr >= pcb[pid].mem_start && ptr + len <= pcb[pid].mem_end;
}

static int32_t sys_write_impl(uint32_t fd, const char *buf, uint32_t len) {
    if (fd != 1 || len > 256) {
        return -2;
    }
    if (!valid_user_range(current_process, (uint32_t)buf, len)) {
        return -3;
    }

    for (uint32_t i = 0; i < len; i++) {
        uart_putc(buf[i]);
    }

    return (int32_t)len;
}

void watchdog_disable(void) {
    // Disable watchdog timer (WDT1) on AM335x
    // The watchdog will reset the board if not disabled or serviced periodically
    // Sequence: Write 0xAAAA to WDT_WSPR, wait for WWPS, then write 0x5555
    
    // First disable sequence
    PUT32(WDT_WSPR, 0xAAAA);
    // Wait for write posting to complete
    while (GET32(WDT_WWPS) != 0);
    
    // Second disable sequence
    PUT32(WDT_WSPR, 0x5555);
    // Wait for write posting to complete
    while (GET32(WDT_WWPS) != 0);
    
    // Verify watchdog is disabled (optional, but good for debugging)
    // WDT_WCLR should be 0 when disabled
}

__attribute__((noinline))
void create_process(uint32_t index_pcb, uint32_t pid, uint32_t entry_point, uint32_t stack_top) {
    uint32_t* sp = (uint32_t*)stack_top;

    // Frame layout shared by IRQ/SVC/fault restore: SPSR, R0-R12, PC.
    sp -= 15;
    for(int i = 0; i < 15; i++) {
        sp[i] = 0;
    }
    sp[TF_SPSR] = USER_MODE_CPSR;
    sp[TF_PC] = entry_point;

    pcb[index_pcb].sp = (uint32_t)sp;
    pcb[index_pcb].user_sp = stack_top - 0x1000;
    pcb[index_pcb].mem_start = entry_point;
    pcb[index_pcb].mem_end = stack_top;
    pcb[index_pcb].pid = pid;
    pcb[index_pcb].state = READY;
}

uint32_t get_current_user_sp(void) {
    return pcb[current_process].user_sp;
}

uint32_t c_context_switch(uint32_t current_sp, uint32_t user_sp) {
    trace_user_to_kernel("timer_irq\n");
    pcb[current_process].sp = current_sp;
    pcb[current_process].user_sp = user_sp;

    timer_irq_handler();
    uint32_t next_process = schedule(0);

    trace_kernel_to_user(next_process, "dispatch\n");

    return pcb[next_process].sp;
}

uint32_t c_svc_handler(uint32_t current_sp, uint32_t user_sp) {
    uint32_t *frame = (uint32_t *)current_sp;
    uint32_t id = frame[TF_R0];
    int32_t rc = 0;

    pcb[current_process].sp = current_sp;
    pcb[current_process].user_sp = user_sp;
    os_write("MODE_SWITCH USER_TO_KERNEL pid=");
    os_write_uint(current_process);
    os_write(" reason=syscall id=");
    os_write_uint(id);
    os_write("\n");

    if ((frame[TF_SPSR] & 0x1F) != USER_MODE_CPSR) {
        rc = -2;
    } else if (id == SYS_YIELD) {
        if (pcb[current_process].state != TERMINATED) {
            pcb[current_process].state = READY;
        }
        rc = 0;
        frame[TF_R0] = (uint32_t)rc;
        schedule(1);
    } else if (id == SYS_EXIT) {
        pcb[current_process].exit_code = (int32_t)frame[TF_R1];
        pcb[current_process].state = TERMINATED;
        rc = 0;
        schedule(1);
    } else if (id == SYS_WRITE) {
        rc = sys_write_impl(frame[TF_R1], (const char *)frame[TF_R2], frame[TF_R3]);
        frame[TF_R0] = (uint32_t)rc;
    } else {
        rc = -1;
        frame[TF_R0] = (uint32_t)rc;
    }

    os_write("MODE_SWITCH KERNEL_TO_USER pid=");
    os_write_uint(current_process);
    os_write(" reason=syscall_return id=");
    os_write_uint(id);
    os_write(" rc=");
    os_write_int(rc);
    os_write("\n");

    return pcb[current_process].sp;
}

uint32_t c_fault_handler(uint32_t current_sp, uint32_t user_sp, uint32_t fault_type) {
    pcb[current_process].sp = current_sp;
    pcb[current_process].user_sp = user_sp;
    pcb[current_process].fault_type = fault_type;
    pcb[current_process].state = TERMINATED;

    os_write("MODE_SWITCH USER_TO_KERNEL pid=");
    os_write_uint(current_process);
    os_write(" reason=fault type=");
    os_write_uint(fault_type);
    os_write("\n");

    uint32_t next = schedule(1);
    trace_kernel_to_user(next, "fault_recovery\n");

    return pcb[next].sp;
}


static uint32_t next_runnable_after(uint32_t pid) {
    const uint32_t user_count = MAX_PROCESSES - FIRST_USER_PID;
    uint32_t start = pid < FIRST_USER_PID ? FIRST_USER_PID : pid + 1;

    for (uint32_t offset = 0; offset < user_count; offset++) {
        uint32_t candidate = FIRST_USER_PID + ((start - FIRST_USER_PID + offset) % user_count);
        if (is_runnable(candidate)) {
            return candidate;
        }
    }

    return PID_OS;
}

uint32_t schedule(uint32_t force_switch) {
    uint32_t next = current_process;

    if (!force_switch && is_runnable(current_process) && rr_ticks_remaining > 1) {
        rr_ticks_remaining--;
        pcb[current_process].state = RUNNING;
        return current_process;
    }

    next = next_runnable_after(current_process);
    if (next == PID_OS) {
        os_write("[OS] No runnable user processes remain. Halting.\n");
        while (1) { }
    }

    if (current_process != next && is_runnable(current_process)) {
        pcb[current_process].state = READY;
    }

    current_process = next;
    pcb[current_process].state = RUNNING;
    rr_ticks_remaining = RR_QUANTUM_TICKS;

    return current_process;
}


// First context switch - from OS to first user process
// This is called once during OS initialization
void first_context_switch(void) {
    current_process = next_runnable_after(PID_OS);
    if (current_process == PID_OS) {
        os_write("[OS] No runnable user processes exist. Halting.\n");
        while (1) { }
    }
    pcb[current_process].state = RUNNING;
    rr_ticks_remaining = RR_QUANTUM_TICKS;
    
    // The assembly code will load P1's context and jump to it
    trace_kernel_to_user(current_process, "initial_launch\n");
    start_process_asm(pcb[current_process].sp);
}

int main(void) {
    os_write("=== OS Starting ===\n");

    for(int i = 0; i < MAX_PROCESSES; i++) {
        pcb[i].state = BLOCKED;
    }
    
    // Disable watchdog first
    watchdog_disable();
    os_write("  - Watchdog disabled\n");
    
    // Initialize PCBs for all processes
    os_write("  - Initializing PCBs...\n");
    
    // OS PCB (PID 0) - not really used for scheduling
    pcb[PID_OS].pid = PID_OS;
    pcb[PID_OS].state = RUNNING;  // OS is always running
    os_write("    - OS PCB initialized\n");

    // P1 PCB (PID 1)
    create_process(PID_P1, PID_P1, P1_ENTRY, P1_STACK_TOP);
    os_write("    - P1 initialized\n");
    
    // P2 PCB (PID 2)
    create_process(PID_P2, PID_P2, P2_ENTRY, P2_STACK_TOP);
    os_write("    - P2 initialized\n");

    // P3 PCB (PID 3)
    create_process(PID_P3, PID_P3, P3_ENTRY, P3_STACK_TOP);
    os_write("    - P3 initialized\n");

    // Initialize timer for periodic interrupts
    timer_init(1000);
    
    os_write("  - Starting first process (P1)...\n");
    os_write("=== Context Switching Started ===\n");
    
    // Trigger first context switch to P1
    // This will never return until processes yield or block
    first_context_switch();
    return 0;
}
