/* smp.h - SMP related stuff - AP entry, IPIs */
#ifndef ARCH_I386_SMP_H
#define ARCH_I386_SMP_H

/* Initialize the AP entry stuff. This moves the AP entry code to the low memory region. */
void init_ap_entry(void);

/* Enumerate APs and start them one by one. */
void start_aps(void);

#endif
