#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <yaos2/kernel/defs.h>

#include <sys/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

pid_t wait(int *status);
pid_t fork(void);
pid_t getpid(void);

int brk(void *ptr);
void *sbrk(int increment);

int open(const char *path, int flags);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
long int lseek(int fd, long int offset, int whence);


#ifdef __cplusplus
}
#endif

#endif
