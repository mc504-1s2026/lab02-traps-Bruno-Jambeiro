#include <arch/csr.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/panic.h>
#include <kernel/serial.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq()
{
	u64 cause = csr_read(CSR_SCAUSE) & ~TRAP_IRQ_BIT;

	switch (cause) {
	case TRAP_TIMER_IRQ & ~TRAP_IRQ_BIT:
		timer_irq();
		break;
	case TRAP_EXTERNAL_IRQ & ~TRAP_IRQ_BIT:
		serial_irq();
		break;
	default:
		panic("unhandled irq cause 0x%x\n", cause);
	}
}

void handle_exception()
{
	u64 cause = csr_read(CSR_SCAUSE);
	u64 stval = csr_read(CSR_STVAL);
	u64 sepc = csr_read(CSR_SEPC);

	panic("unhandled exception cause=0x%x stval=0x%x sepc=0x%x\n",
	      cause, stval, sepc);
}

void trap_setup()
{
	csr_write(CSR_STVEC, (u64)trap_entry);
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
	csr_write(CSR_SIE, 0);
}

void handle_trap()
{
	u64 cause = csr_read(CSR_SCAUSE);

	if (cause & TRAP_IRQ_BIT)
		handle_irq();
	else
		handle_exception();
}

void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
	u64 flags = csr_read(CSR_SSTATUS) & CSR_SSTATUS_SIE;
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
	return flags;
}

void hart_irq_restore(u64 flags)
{
	if (flags & CSR_SSTATUS_SIE)
		csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
	else
		csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}
