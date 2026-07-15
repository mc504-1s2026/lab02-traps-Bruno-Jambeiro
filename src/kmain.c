#include <arch/timer.h>
#include <kernel/mm.h>
#include <kernel/printf.h>
#include <kernel/serial.h>
#include <kernel/string.h>
#include <kernel/trap.h>

extern int _hartid[];

static volatile u8 alarm_pending = 0;

static void shell_print_u64(u64 value)
{
	char buf[32];
	size_t i = 0;

	if (value == 0) {
		serial_putc('0');
		return;
	}

	while (value > 0) {
		buf[i++] = (char)('0' + (value % 10));
		value /= 10;
	}

	while (i > 0)
		serial_putc(buf[--i]);
}

static void shell_execute(char *cmd)
{
	if (strcmp(cmd, "uptime") == 0) {
		serial_puts("\r\n");
		shell_print_u64(timer_read() / TIMER_FREQ);
		serial_puts("s\r\n");
	} else if (strncmp(cmd, "echo ", 5) == 0) {
		serial_puts("\r\n");
		serial_puts(cmd + 5);
		serial_puts("\r\n");
	} else if (strncmp(cmd, "alarm ", 6) == 0) {
		u64 secs = strtou64(cmd + 6, 10);
		serial_puts("\r\n");
		timer_set_alarm(secs);
	} else if (cmd[0] != '\0') {
		serial_puts("\r\nunknown command\r\n");
	}
}

void kmain()
{
	char line[128];
	size_t len = 0;

	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();
	hart_irq_enable();

	serial_puts("> ");

	for (;;) {
		char buf[64];
		size_t count = serial_read(buf);
		for (size_t i = 0; i < count; i++) {
			char c = buf[i];
			if (c == '\r' || c == '\n') {
				if (len > 0) {
					line[len] = '\0';
					serial_puts("\r\n");
					shell_execute(line);
					len = 0;
				} else {
					serial_puts("\r\n");
				}
				serial_puts("> ");
			} else if (c == '\b' || c == 0x7f) {
				if (len > 0) {
					len--;
					serial_puts("\b \b");
				}
			} else if (len < sizeof(line) - 1) {
				line[len++] = c;
				serial_putc(c);
			}
		}

		if (alarm_pending) {
			alarm_pending = 0;
			serial_puts("alarm\r\n");
			serial_puts("> ");
		}
	}
}
