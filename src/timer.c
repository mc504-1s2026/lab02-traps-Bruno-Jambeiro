#include <arch/csr.h>
#include <arch/timer.h>
#include <kernel/panic.h>
#include <kernel/serial.h>

static volatile u8 alarm_pending = 0;

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_STIE);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	u64 now = timer_read();
	u64 deadline = now + (secs * TIMER_FREQ);

	alarm_pending = 1;
	csr_write(CSR_STIMECMP, deadline);
	timer_irq_enable();
}

void timer_irq()
{
	if (alarm_pending) {
		alarm_pending = 0;
		serial_puts("alarm\r\n");
	}

	timer_irq_disable();
}
