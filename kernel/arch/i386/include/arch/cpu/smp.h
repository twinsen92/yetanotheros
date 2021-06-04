/* cpu/smp.h - SMP related stuff - AP entry, IPIs */
#ifndef ARCH_I386_CPU_SMP_H
#define ARCH_I386_CPU_SMP_H

/* Initialize SMP stuff. */
void init_smp(void);

/* Enumerate APs and start them one by one. */
void start_aps(void);

#endif
