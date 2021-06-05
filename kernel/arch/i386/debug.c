/* debug.c - x86 kernel/debug.h implementation */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/exclusive_buffer.h>
#include <kernel/heap.h>
#include <kernel/printf.h>
#include <kernel/thread.h>
#include <kernel/devices/serial.h>
#include <arch/cpu.h>
#include <arch/memlayout.h>
#include <arch/serial.h>
#include <arch/thread.h>
#include <arch/vga.h>

static struct cpu_spinlock vga_kdprintf_spinlock;

static struct thread_mutex serial_mutex;
static atomic_bool debug_serial_enabled;
static struct serial *debug_serial = NULL;
static struct exclusive_buffer *debug_buffer = NULL;

/* Initializes the debug subsystem in the kernel. Call this early in initialization. */
void init_debug(void)
{
	init_vga_printf();
	cpu_spinlock_create(&vga_kdprintf_spinlock, "VGA kdprintf spinlock");
	thread_mutex_create(&serial_mutex);
}

/* Redirects debug output to the given serial port. */
void debug_redirect_to_serial(struct serial *s)
{
	kassert(s);

	thread_mutex_acquire(&serial_mutex);

	if (debug_serial == s)
		goto redundant;

	if (debug_serial)
	{
		/* A buffer was already allocated and it is subscribed to a serial output. */
		serial_unsubscribe_output(debug_serial, debug_buffer);
	}
	else
	{
		/* The buffer has to be allocated. */
		debug_buffer = kalloc(HEAP_NORMAL, 1, sizeof(struct exclusive_buffer));
		eb_create(debug_buffer, 512, 1, 0);
	}

	/* Subscribe to the new serial port. */
	debug_serial = s;
	serial_subscribe_output(debug_serial, debug_buffer);

	atomic_store(&debug_serial_enabled, true);

redundant:
	thread_mutex_release(&serial_mutex);
}

/* Wait for debug output to be flushed. */
void debug_flush(void)
{
	if (atomic_load(&debug_serial_enabled))
	{
		eb_flush(debug_buffer);
	}
}

static void unsafe_serial_putchar(char c)
{
	eb_try_write(debug_buffer, (uint8_t*)&c, 1);
}

/* Debug formatted printer. */
int kdprintf(const char * restrict format, ...)
{
	int ret;
	va_list parameters;

	va_start(parameters, format);

	if (atomic_load(&debug_serial_enabled))
	{
		thread_mutex_acquire(&serial_mutex);

		/* We use the multi-lock feature of exclusive_buffer to write all characters to the buffer
		   one by one. */
		eb_lock(debug_buffer);
		ret = generic_printf(unsafe_serial_putchar, format, parameters);
		eb_unlock(debug_buffer);

		/* Stimulate a write. */
		serial_write(debug_serial);

		thread_mutex_release(&serial_mutex);
	}
	else
	{
		cpu_spinlock_acquire(&vga_kdprintf_spinlock);
		ret = va_vga_printf(format, parameters);
		cpu_spinlock_release(&vga_kdprintf_spinlock);
	}

	va_end(parameters);

	return ret;
}

packed_struct stack_frame
{
	struct stack_frame *ebp;
	void *eip;
};

static void get_stack_info(void **top, void **bottom)
{
	struct x86_cpu *cpu;

	cpu = cpu_current_or_null();

	if (cpu == NULL)
	{
		/* We're in early initialization stages. Return the boot stack shape. */
		*top = get_symbol_vaddr(__boot_stack_top);
		*bottom = get_symbol_vaddr(__boot_stack_bottom);

		return;
	}

	if (cpu->thread && cpu->thread->stack0)
	{
		*top = cpu->thread->stack0;
		*bottom = cpu->thread->stack0 + cpu->thread->stack0_size;
	}
	else if (cpu->thread && cpu->thread->stack)
	{
		*top = cpu->thread->stack;
		*bottom = cpu->thread->stack + cpu->thread->stack_size;
	}
	else
	{
		kassert(cpu->thread == NULL);
		*top = cpu->stack_top;
		*bottom = cpu->stack_top + cpu->stack_size;
	}
}

/* Clears and fills the call stack with current data. */
void debug_fill_call_stack(struct debug_call_stack *cs)
{
	void *stack_top;
	void *stack_bottom;
	struct stack_frame *frame;
	unsigned int i;

	/* NOTE: We assume cdecl calling convention. We also assume 32 bits. */
	kassert(sizeof(void*) == sizeof(uint32_t));

	debug_clear_call_stack(cs);

	/* Need to turn off interrupts so that we're not rescheduled during this process. Stack pointers
	   might change if we're called outside of thread context. */
	push_no_interrupts();

	/* Try to get the current stack shape. This might get called outside of thread context so we
	   also look at the current CPU for clues. The stack shape will help us detect when we've
	   reached the end of the call stack. */
	get_stack_info(&stack_top, &stack_bottom);

	/* Get current call frame's EBP. It will point at the previous call frame's EBP. Right behind
	   it is the return address. */
	asm volatile ("movl %%ebp, %0" : "=r" (frame));

	for (i = 0; i < DEBUG_MAX_CALL_DEPTH; i++)
	{
		/* Check if we're outside of the stack. */
		if ((void*)frame > stack_bottom || (void*)frame < stack_top)
			break;

		cs->stack[i] = frame->eip;
		frame = frame->ebp;
	}

	cs->depth = i;

	pop_no_interrupts();
}
