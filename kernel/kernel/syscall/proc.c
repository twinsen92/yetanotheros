/* kernel/syscall.c - syscall subsystem */
#include <kernel/cdefs.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>

#include <user/yaos2/kernel/errno.h>

/* Exits the process. */
noreturn syscall_exit(int status)
{
	proc_exit(status);
}

pid_t syscall_wait(uvaddr_t status)
{
	struct proc *proc;
	int kstatus = -ENOSTATUS;
	pid_t ret = -EUNSPEC;
	int *ustatus;

	/* Wait with kernel virtual memory. */
	proc = get_current_proc();
	proc_set_kvm();
	ret = proc_wait(&kstatus);
	proc_set_uvm(proc);

	/* Write to user space. TODO: Make sure it is accessible. */
	ustatus = (int *)status;
	*ustatus = kstatus;

	return ret;
}

uvaddr_t syscall_getenvptr(void)
{
	struct proc *proc;
	uvaddr_t ret = -EUNSPEC;

	proc = get_current_proc();
	proc_set_kvm();
	ret = proc_get_env(proc);
	proc_set_uvm(proc);

	return ret;
}

pid_t syscall_getpid(void)
{
	return get_current_proc()->pid;
}
