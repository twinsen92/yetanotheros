
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>

int syscall_brk(uvaddr_t ptr)
{
	struct thread *thread;
	struct proc *proc;
	int ret;

	thread = get_current_thread();

	if (thread == NULL)
		kpanic("syscall_brk(): null thread");

	proc = thread->parent;

	if (proc == NULL)
		kpanic("syscall_brk(): null proc");

	proc_set_kvm();
	ret = proc_brk(proc, ptr);
	proc_set_uvm(proc);

	return ret;
}

uvaddr_t syscall_sbrk(uvaddrdiff_t diff)
{
	struct thread *thread;
	struct proc *proc;
	int ret;

	thread = get_current_thread();

	if (thread == NULL)
		kpanic("syscall_sbrk(): null thread");

	proc = thread->parent;

	if (proc == NULL)
		kpanic("syscall_sbrk(): null proc");

	proc_set_kvm();
	ret = proc_sbrk(thread->parent, diff);
	proc_set_uvm(proc);

	return ret;
}
