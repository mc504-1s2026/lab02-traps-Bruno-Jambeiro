#include <arch/csr.h>
#include <arch/io.h>
#include <arch/plic.h>
#include <kernel/serial.h>
#include <kernel/string.h>
#include <kernel/trap.h>

#define SERIAL_BUF_SIZE 256

struct serial_state {
	u8 buf[SERIAL_BUF_SIZE];
	size_t head;
	size_t tail;
	size_t len;
	bool initialized;
};

static struct serial_state serial_state;

static inline u8 serial_reg_read(u32 offset)
{
	return ioread8((u8 *)SERIAL_BASE + offset);
}

static inline void serial_reg_write(u32 offset, u8 value)
{
	iowrite8(value, (u8 *)SERIAL_BASE + offset);
}

void serial_init()
{
	if (serial_state.initialized)
		return;

	serial_reg_write(SERIAL_IER, 0x00);
	serial_reg_write(SERIAL_LCR, 0x03);
	serial_reg_write(SERIAL_FCR, SERIAL_FCR_FIFO_ENABLE |
				     SERIAL_FCR_RX_FIFO_CLEAR |
				     SERIAL_FCR_TX_FIFO_CLEAR);
	serial_reg_write(SERIAL_MCR, 0x00);

	memset(&serial_state, 0, sizeof(serial_state));
	serial_state.initialized = true;

	plic_irq_set_priority(IRQ_SERIAL, 7);
	plic_hart_enable_irq(0, IRQ_SERIAL);
	plic_hart_set_threshold(0, 0);
}

void serial_irq_enable()
{
	if (!serial_state.initialized)
		serial_init();

	serial_reg_write(SERIAL_IER, serial_reg_read(SERIAL_IER) | SERIAL_IER_ERBFI);
	csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{
	serial_reg_write(SERIAL_IER, serial_reg_read(SERIAL_IER) & ~SERIAL_IER_ERBFI);
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	while ((serial_reg_read(SERIAL_LSR) & SERIAL_LSR_DTR) != 0 &&
	       serial_state.len < SERIAL_BUF_SIZE) {
		u8 c = serial_reg_read(SERIAL_RBR);
		serial_state.buf[serial_state.tail] = c;
		serial_state.tail = (serial_state.tail + 1) % SERIAL_BUF_SIZE;
		serial_state.len++;
	}
}

size_t serial_read(char *buf)
{
	size_t count;
	u64 flags = hart_irq_save();

	count = serial_state.len;
	for (size_t i = 0; i < count; i++) {
		buf[i] = serial_state.buf[serial_state.head];
		serial_state.head = (serial_state.head + 1) % SERIAL_BUF_SIZE;
	}
	serial_state.len = 0;
	serial_state.head = 0;
	serial_state.tail = 0;

	hart_irq_restore(flags);
	return count;
}

void serial_puts(char *str)
{
	while (*str != '\0')
		serial_putc(*str++);
}

void serial_putc(char c)
{
	while ((serial_reg_read(SERIAL_LSR) & SERIAL_LSR_THRE) == 0)
		;
	serial_reg_write(SERIAL_THR, (u8)c);
}
