/* kernel/endian.h - endianness utility macros */
#ifndef _KERNEL_ENDIAN_H
#define _KERNEL_ENDIAN_H

#include <kernel/cdefs.h>

#define LITTLE_ENDIAN 1
#define BIG_ENDIAN 2

#define CURRENT_ENDIANNESS (!*(unsigned char *)&(uint16_t){1} ? BIG_ENDIAN : LITTLE_ENDIAN)

#define spin_uint64(x) ((uint64_t)(((x & ((uint64_t)0xff << 56)) >> 56) \
						| ((x & ((uint64_t)0xff << 48)) >> 40) \
						| ((x & ((uint64_t)0xff << 40)) >> 24) \
						| ((x & ((uint64_t)0xff << 32)) >> 8) \
						| ((x & ((uint64_t)0xff << 24)) << 8) \
						| ((x & ((uint64_t)0xff << 16)) << 24) \
						| ((x & ((uint64_t)0xff << 8)) << 40) \
						| ((x & (uint64_t)0xff) << 56)))

#define spin_uint32(x) ((uint32_t)(((x & ((uint32_t)0xff << 24)) >> 24) \
						| ((x & ((uint32_t)0xff << 16)) >> 8) \
						| ((x & ((uint32_t)0xff << 8)) << 8) \
						| ((x & (uint32_t)0xff) << 24)))

#define spin_uint16(x) ((uint16_t)(((x & (uint16_t)0xff00) >> 8) | ((x & (uint16_t)0xff) << 8)))

#define little_endian_uint64(x) (CURRENT_ENDIANNESS == LITTLE_ENDIAN ? x : spin_uint64(x))
#define big_endian_uint64(x) (CURRENT_ENDIANNESS == BIG_ENDIAN ? x : spin_uint64(x))
#define little_endian_uint32(x) (CURRENT_ENDIANNESS == LITTLE_ENDIAN ? x : spin_uint32(x))
#define big_endian_uint32(x) (CURRENT_ENDIANNESS == BIG_ENDIAN ? x : spin_uint32(x))
#define little_endian_uint16(x) (CURRENT_ENDIANNESS == LITTLE_ENDIAN ? x : spin_uint16(x))
#define big_endian_uint16(x) (CURRENT_ENDIANNESS == BIG_ENDIAN ? x : spin_uint16(x))

#endif
