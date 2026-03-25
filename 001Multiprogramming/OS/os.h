#ifndef OS_H
#define OS_H

// Low-level OS interface functions
void os_write(const char *s);

// UART helper functions
void uart_putc(char c);

// Watchdog functions
void disable_watchdog(void);

// Timer functions
void timer_init(void);
void timer_irq_handler(void);

// Process / scheduler functions
void process_init(void);
unsigned int schedule_next_sp(unsigned int current_sp);
void start_first_task(unsigned int sp);

// Interrupt control
void enable_irq(void);

// Low-level memory access functions (implemented in root.s)
void PUT32(unsigned int addr, unsigned int value);
unsigned int GET32(unsigned int addr);

#endif // OS_H