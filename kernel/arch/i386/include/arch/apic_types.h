/* apic_types.h - APIC related types and constants */
#ifndef ARCH_I386_APIC_TYPES_H
#define ARCH_I386_APIC_TYPES_H

#include <kernel/cdefs.h>

typedef uint8_t lapic_id_t;
typedef uint32_t lapic_reg_t;

#define LAPIC_REG_ID			(0x0020 / sizeof (lapic_reg_t))   // ID
#define LAPIC_REG_VER			(0x0030 / sizeof (lapic_reg_t))   // Version
#define LAPIC_REG_TPR			(0x0080 / sizeof (lapic_reg_t))   // Task Priority
#define LAPIC_REG_EOI			(0x00B0 / sizeof (lapic_reg_t))   // EOI
#define LAPIC_REG_SVR			(0x00F0 / sizeof (lapic_reg_t))   // Spurious Interrupt Vector
#define LAPIC_REG_ESR			(0x0280 / sizeof (lapic_reg_t))   // Error Status
#define LAPIC_REG_ICRLO			(0x0300 / sizeof (lapic_reg_t))   // Interrupt Command

#define LAPIC_ICR_INIT			0x00000500   // INIT/RESET
#define LAPIC_ICR_STARTUP		0x00000600   // Startup IPI
#define LAPIC_ICR_DELIVS		0x00001000   // Delivery status
#define LAPIC_ICR_ASSERT		0x00004000   // Assert interrupt (vs deassert)
#define LAPIC_ICR_DEASSERT		0x00000000
#define LAPIC_ICR_LEVEL			0x00008000   // Level triggered
#define LAPIC_ICR_BCAST			0x00080000   // Send to all APICs, including self.
#define LAPIC_ICR_BUSY			0x00001000
#define LAPIC_ICR_FIXED			0x00000000

#define LAPIC_REG_ICRHI			(0x0310 / sizeof (lapic_reg_t))   // Interrupt Command [63:32]
#define LAPIC_REG_TIMER			(0x0320 / sizeof (lapic_reg_t))   // Local Vector Table 0 (TIMER)

#define LAPIC_TIMER_X1			0x0000000B   // divide counts by 1
#define LAPIC_TIMER_PERIODIC	0x00020000   // Periodic

#define LAPIC_REG_PCINT			(0x0340 / sizeof (lapic_reg_t))   // Performance Counter LVT
#define LAPIC_REG_LINT0			(0x0350 / sizeof (lapic_reg_t))   // Local Vector Table 1 (LINT0)
#define LAPIC_REG_LINT1			(0x0360 / sizeof (lapic_reg_t))   // Local Vector Table 2 (LINT1)
#define LAPIC_REG_ERROR			(0x0370 / sizeof (lapic_reg_t))   // Local Vector Table 3 (ERROR)
#define LAPIC_REG_TICR			(0x0380 / sizeof (lapic_reg_t))   // Timer Initial Count
#define LAPIC_REG_TCCR			(0x0390 / sizeof (lapic_reg_t))   // Timer Current Count
#define LAPIC_REG_TDCR			(0x03E0 / sizeof (lapic_reg_t))   // Timer Divide Configuration

#define LAPIC_ENABLE			0x00000100   // Unit Enable
#define LAPIC_MASKED			0x00010000   // Interrupt masked

#endif
