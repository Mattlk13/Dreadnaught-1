// timer.c -- Brad Slayter

#include "lib/timer.h"

#include "kernel/isr.h"
#include "kernel/task.h"

#include "io/monitor.h"

u32int tick = 0;

static void timer_callback(registers_t *regs) {
	tick++;
	task_switch();
}

void init_timer(u32int freq) {
	register_interrupt_handler(IRQ0, &timer_callback);

	u32int divisor = 1193180 / freq;

	outb(0x43, 0x36);

	u8int l = (u8int)(divisor & 0xFF);
	u8int h = (u8int)((divisor >> 8) & 0xFF);

	outb(0x40, l);
	outb(0x40, h);
}

void reset_tick_count() {
	tick = 0;
}

void sleep(int ms) {
	u32int target = tick + ms;

	while (tick < target)
		;
}