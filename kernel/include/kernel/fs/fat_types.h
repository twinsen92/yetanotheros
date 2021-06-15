/* kernel/fs/fat_types.h - FAT filesystem declarations and constants */
#ifndef _KERNEL_FS_FAT_TYPES_H
#define _KERNEL_FS_FAT_TYPES_H

#include <kernel/cdefs.h>
#include <kernel/vfs.h>
#include <kernel/wchar.h>

#define FAT_ENTRY_SIZE 32
#define FAT_CLUSTER_OFFSET 2

/* FAT32 boot sector data */
packed_struct boot_fat_ext_32
{
	uint32_t table_size_32;
	uint16_t extended_flags;
	uint16_t fat_version;
	uint32_t root_cluster;
	uint16_t fat_info;
	uint16_t backup_BS_sector;
	uint8_t reserved_0[12];
	uint8_t drive_number;
	uint8_t reserved_1;
	uint8_t boot_signature;
	uint32_t volume_id;
	char volume_label[11];
	char fat_type_label[8];
};

/* FAT12 and FAT16 boot sector data */
packed_struct boot_fat_ext_16
{
	uint8_t bios_drive_num;
	uint8_t reserved1;
	uint8_t boot_signature;
	uint32_t volume_id;
	char volume_label[11];
	char fat_type_label[8];
};

/* FAT boot (first) sector. */
packed_struct fat_bootsec
{
	uint8_t bootjmp[3];
	char oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t table_count;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media_type;
	uint16_t table_size_16;
	uint16_t sectors_per_track;
	uint16_t head_side_count;
	uint32_t hidden_sector_count;
	uint32_t total_sectors_32;

	union
	{
		struct boot_fat_ext_16 ext16;
		struct boot_fat_ext_32 ext32;
	};
};

typedef enum fat_type
{
	FAT12,
	FAT16,
	FAT32,
	ExFAT,
} fat_type_t;

#define FAT_LAST_ENTRY 0
#define FAT_UNUSED_ENTRY 0xe5

/* FAT directory entry */

#define FAT_READ_ONLY 0x01
#define FAT_HIDDEN 0x02
#define FAT_SYSTEM 0x04
#define FAT_VOLUME_ID 0x08
#define FAT_DIRECTORY 0x10
#define FAT_ARCHIVE 0x20

packed_struct fat_entry
{
	char file_name[11];
	uint8_t attributes;

	uint8_t windows_nt;

	uint8_t create_usec;
	uint16_t create_hms;
	uint16_t create_ymd;

	uint16_t access_ymd;

	uint16_t cluster_high;

	uint16_t modified_hms;
	uint16_t modified_ymd;

	uint16_t cluster_low;

	uint32_t num_bytes;
};

#define fat_hour(field16) ((field16 & 0xf800) >> 11)
#define fat_minute(field16) ((field16 & 0x7e0) >> 5)
#define fat_second(field16) (field16 & 0x1f)

#define fat_year(field16) ((field16 & 0xfe00) >> 9)
#define fat_month(field16) ((field16 & 0x1e0) >> 5)
#define fat_day(field16) (field16 & 0x1f)

/* FAT "long file name" directory entry */

#define FAT_LFN_DELETED 0x80
#define FAT_LFN_LAST 0x40

#define FAT_LFN 0x0f
#define FAT_LFN_NAME 0

#define FAT_LFN_CHARS 13
#define FAT_LFN_NAME_SIZE 256

packed_struct fat_lfn
{
	uint8_t ordinal;

	ucs2_t chars0[5];

	uint8_t signature;
	uint8_t type;
	uint8_t checksum;

	ucs2_t chars1[6];

	uint16_t reserved;

	ucs2_t chars2[2];
};

#define fat_is_lfn_deleted(lfn) ((lfn->ordinal & FAT_LFN_DELETED) != 0)
#define fat_get_lfn_nr(lfn) (lfn->ordinal & 0x3f)
#define fat_is_lfn_entry(entry) (((struct fat_lfn*)(entry))->signature == FAT_LFN)

/* Get the first cluster of a file from its entry. */
static inline uint32_t fat_get_entry_cluster(const struct fat_entry *entry)
{
	return ((uint32_t)(entry->cluster_high) << 16) | ((uint32_t)entry->cluster_low);
}

/* Convert UCS2 to ASCII and append to the given pointer. */
static inline char *fat_append_ascii_chars(char *to, const ucs2_t *from, int num)
{
	int i;

	for (i = 0; i < num; i++)
	{
		to[i] = ucs2_to_ascii(from[i]);
	}

	return to + num;
}

/* Convert all UCS2 entries of a LFN entry to ASCII and append to the given pointer. */
static inline void fat_copy_all_ascii_chars(char *to, const struct fat_lfn *lfn)
{
	to = fat_append_ascii_chars(to, lfn->chars0, 5);
	to = fat_append_ascii_chars(to, lfn->chars1, 6);
	to = fat_append_ascii_chars(to, lfn->chars2, 2);
}

/* FAT core functions (kernel/fs/fat_core.c) */

#define FAT_OK 0
#define FAT_LAST_CLUSTER 1
#define FAT_BAD_CLUSTER 2
#define FAT_ERROR 3

uint fat_read_fat(struct vfs_super *super, uint32_t *next_cluster, uint32_t cluster);

uint32_t fat_walk_path(struct fat_entry *result, struct vfs_super *super, const char *path,
	uint32_t root_cluster, char *name_buffer);

int fat_read(struct vfs_super *super, uint32_t first_cluster, void *buf, uint off,
	int num);

uint32_t fat_read_entry(struct fat_entry *result, struct vfs_super *super, uint32_t first_cluster,
	uint32_t idx, char *name_buffer);

#endif
