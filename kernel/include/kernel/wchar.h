/* kernel/wchar.h - wide character utilities */
#ifndef _KERNEL_WCHAR_H_
#define _KERNEL_WCHAR_H_

#include <kernel/cdefs.h>

typedef uint16_t ucs2_t;

#define ucs2_to_ascii(ucs2_char) ((char)(ucs2_char))

#endif
