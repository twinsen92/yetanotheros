/* arch/i386/syscall/proc.c - x86 process management system calls implementation */
#include <arch/interrupts.h>
#include <arch/proc.h>
#include <arch/thread.h>

#include <kernel/proc.h>
#include <kernel/scheduler.h>

#include <user/yaos2/kernel/types.h>

pid_t syscall_fork(struct isr_frame *frame)
{
	struct proc *proc, *np;
	struct thread *nt;
	pid_t ret;

	proc = get_current_proc();
	proc_set_kvm();
	np = proc_fork(&nt, frame);
	ret = schedule_proc(proc, np, nt);
	proc_set_uvm(proc);

	return ret;
}
