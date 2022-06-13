#ifndef _UNISTD_H_
#define _UNISTD_H_

#ifdef __cplusplus
extern "C" {
#endif

int brk(void *ptr);
void *sbrk(int increment);

#ifdef __cplusplus
}
#endif

#endif
