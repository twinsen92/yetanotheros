#ifndef _KERNEL_EXEC_ELF_H
#define _KERNEL_EXEC_ELF_H

void exec_user_elf_program(const char *path, const char *stdin, const char *stdout, const char *stderr);

#endif
