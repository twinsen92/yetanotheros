/* cpu.h - arch independent per CPU system interface */
#ifndef _KERNEL_CPU_H
#define _KERNEL_CPU_H

/* Get the number of CPUs. */
unsigned int get_nof_cpus(void);

/* Get the number of active CPUs. */
unsigned int get_nof_active_cpus(void);

#endif
