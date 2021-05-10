/* selectors.h - YAOS2 x86 GDT selectors */
#ifndef ARCH_I386_SELECTORS_H
#define ARCH_I386_SELECTORS_H

#define YAOS2_GDT_NOF_ENTRIES 6

#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define KERNEL_TSS_SELECTOR 0x28
#define USER_CODE_SELECTOR 0x1B
#define USER_DATA_SELECTOR 0x23
#define USER_TSS_SELECTOR 0x2B

#endif