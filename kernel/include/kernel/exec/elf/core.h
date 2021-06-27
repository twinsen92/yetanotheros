/* kernel/exec/elf/core.h - core ELF structures and definitions */
#ifndef _KERNEL_EXEC_ELF_CORE_H
#define _KERNEL_EXEC_ELF_CORE_H

#include <kernel/cdefs.h>

#define ELF_BITS_32 1
#define ELF_BITS_64 2

#define ELF_LITTLE_ENDIAN 1
#define ELF_BIG_ENDIN 2

#define ELF_TYPE_RELOC 1
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_SHARED 3
#define ELF_TYPE_CORE 4

#define ELF_ISET_NONE 0
#define ELF_ISET_SPARC 2
#define ELF_ISET_X86 3
#define ELF_ISET_MIPS 8
#define ELF_ISET_PPC 0x14
#define ELF_ISET_ARM 0x28
#define ELF_ISET_SUPERH 0x2A
#define ELF_ISET_ITANIUM 0x32
#define ELF_ISET_AMD64 0x3E
#define ELF_ISET_ARCH64 0xB7

#define ELF_MAGIC "\x7f""ELF"

packed_struct elf_header
{
	uint8_t magic[4];
	uint8_t bits;
	uint8_t endianness;
	uint8_t file_version;
	uint8_t abi;
	uint8_t _padding_0[8];
	uint16_t type;
	uint16_t iset;
	uint32_t elf_version;
};

packed_struct elf_32_header
{
	uint32_t pe_pos; /* Virtual address of the program entry. */
	uint32_t pht_pos; /* Offset of the program header table. */
	uint32_t sht_pos; /* Offset of the section header table. */

	uint32_t flags; /* Architecture dependent flags. */

	uint16_t header_size; /* Size of the ELF file header. */
	uint16_t phe_size; /* Size of an entry of the program header table. */
	uint16_t pht_len; /* Length of the program header table. */

	uint16_t she_size; /* Size of an entry of the section header table. */
	uint16_t sht_len; /* Length of the section header table. */
	uint16_t sht_index; /* Index in the section header table with section names. */
};

#define ELF_SEG_NULL    0
#define ELF_SEG_LOAD    1
#define ELF_SEG_DYNAMIC 2
#define ELF_SEG_INTERP  3
#define ELF_SEG_NOTE    4

#define ELF_FLAG_EXEC   1
#define ELF_FLAG_WRITE  2
#define ELF_FLAG_READ   4

struct elf_32_program_header
{
	uint32_t type;
	uint32_t file_offset;
	uint32_t mem_offset;
	uint32_t undefined;
	uint32_t file_size;
	uint32_t mem_size;
	uint32_t flags;
	uint32_t align;
};

#define ELF_SHT_NULL        0
#define ELF_SHT_PROGBITS    1
#define ELF_SHT_SYMTAB      2
#define ELF_SHT_STRTAB      3
#define ELF_SHT_RELA        4
#define ELF_SHT_HASH        5
#define ELF_SHT_DYNAMIC     6
#define ELF_SHT_NOTE        7
#define ELF_SHT_NOBITS      8
#define ELF_SHT_REL         9
#define ELF_SHT_SHLIB       0x0a
#define ELF_SHT_DYNSYM      0x0b
#define ELF_SHT_INIT        0x0e
#define ELF_SHT_FINI        0x0f
#define ELF_SHT_PREINIT     0x10
#define ELF_SHT_GROUP       0x11
#define ELF_SHT_SHNDX       0x12
#define ELF_SHT_NUM         0x13

#define ELF_SHF_WRITE       0x1
#define ELF_SHF_ALLOC       0x2
#define ELF_SHF_EXEC        0x4
#define ELF_SHF_MERGE       0x10
#define ELF_SHF_STRINGS     0x20
#define ELF_SHF_INFO_LINK   0x40
#define ELF_SHF_LINK_ORDER  0x80
#define ELF_SHF_GROUP       0x200
#define ELF_SHF_TLS         0x400

struct elf_32_section_header
{
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t mem_offset;
	uint32_t file_offset;
	uint32_t file_size;
	uint32_t link;
	uint32_t info;
	uint32_t align;
	uint32_t ent_size;
};

#endif
