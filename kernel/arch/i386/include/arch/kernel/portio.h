/* arch/kernel/portio.h - functions used to do port I/O on x86 */
#ifndef ARCH_I386_KERNEL_PORTIO_H
#define ARCH_I386_KERNEL_PORTIO_H

#include <kernel/cdefs.h>

static inline void pio_outb(uint16_t port, uint8_t val)
{
	asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t pio_inb(uint16_t port)
{
	uint8_t ret;
	asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void pio_outw(uint16_t port, uint16_t val)
{
	asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t pio_inw(uint16_t port)
{
	uint16_t ret;
	asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void pio_outl(uint16_t port, uint32_t val)
{
	asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t pio_inl(uint16_t port)
{
	uint32_t ret;
	asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

#endif
