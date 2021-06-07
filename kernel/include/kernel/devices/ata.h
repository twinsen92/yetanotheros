/* kernel/devices/ata.h - declarations and definitions for ATA disk drivers */
#ifndef _KERNEL_DEVICES_ATA_H
#define _KERNEL_DEVICES_ATA_H

#include <kernel/cdefs.h>
#include <kernel/thread.h>

/* ATA protocol constants. */

/* ATA statuses. */
#define ATA_SR_BSY					0x80 /* Busy */
#define ATA_SR_DRDY					0x40 /* Drive ready */
#define ATA_SR_DF					0x20 /* Drive write fault */
#define ATA_SR_DSC					0x10 /* Drive seek complete */
#define ATA_SR_DRQ					0x08 /* Data request ready */
#define ATA_SR_CORR					0x04 /* Corrected data */
#define ATA_SR_IDX					0x02 /* Inlex */
#define ATA_SR_ERR					0x01 /* Error */

/* ATA error codes. */
#define ATA_ER_BBK					0x80 /* Bad sector */
#define ATA_ER_UNC					0x40 /* Uncorrectable data */
#define ATA_ER_MC					0x20 /* No media */
#define ATA_ER_IDNF					0x10 /* ID mark not found */
#define ATA_ER_MCR					0x08 /* No media */
#define ATA_ER_ABRT					0x04 /* Command aborted */
#define ATA_ER_TK0NF				0x02 /* Track 0 not found */
#define ATA_ER_AMNF					0x01 /* No address mark */
/* YAOS additional error codes. */
#define ATA_ER_UNSUP				0xff /* Unsupported operation. */
#define ATA_ER_NOERR				0x00 /* No error. */

/* ATA commands */
#define ATA_CMD_READ_PIO			0x20
#define ATA_CMD_READ_PIO_EXT		0x24
#define ATA_CMD_READ_DMA			0xC8
#define ATA_CMD_READ_DMA_EXT		0x25
#define ATA_CMD_WRITE_PIO			0x30
#define ATA_CMD_WRITE_PIO_EXT		0x34
#define ATA_CMD_WRITE_DMA			0xCA
#define ATA_CMD_WRITE_DMA_EXT		0x35
#define ATA_CMD_CACHE_FLUSH			0xE7
#define ATA_CMD_CACHE_FLUSH_EXT		0xEA
#define ATA_CMD_PACKET				0xA0
#define ATA_CMD_IDENTIFY_PACKET		0xA1
#define ATA_CMD_IDENTIFY			0xEC

/* ATAPI commands. */
#define ATAPI_CMD_READ				0xA8
#define ATAPI_CMD_EJECT				0x1B

/* ATA identification field indices. */
#define ATA_IDENT_DEVICETYPE		(0 / sizeof(uint16_t))
#define ATA_IDENT_CYLINDERS			(2 / sizeof(uint16_t))
#define ATA_IDENT_HEADS				(6 / sizeof(uint16_t))
#define ATA_IDENT_SECTORS			(12 / sizeof(uint16_t))
#define ATA_IDENT_SERIAL			(20 / sizeof(uint16_t))
#define ATA_IDENT_FIRMWARE			(46 / sizeof(uint16_t))
#define ATA_IDENT_MODEL				(54 / sizeof(uint16_t))
#define ATA_IDENT_CAPABILITIES		(98 / sizeof(uint16_t))
#define ATA_IDENT_FIELDVALID		(106 / sizeof(uint16_t))
#define ATA_IDENT_MAX_LBA			(120 / sizeof(uint16_t))
#define ATA_IDENT_FCS1				(164 / sizeof(uint16_t))
#define ATA_IDENT_FCS2				(166 / sizeof(uint16_t))
#define ATA_IDENT_FCS3				(168 / sizeof(uint16_t))
#define ATA_IDENT_FCS4				(170 / sizeof(uint16_t))
#define ATA_IDENT_FCS5				(172 / sizeof(uint16_t))
#define ATA_IDENT_FCS6				(174 / sizeof(uint16_t))
#define ATA_IDENT_MAX_LBA_EXT		(200 / sizeof(uint16_t))

/* BAR 0/1 */
#define ATA_REG_DATA				0x00
#define ATA_REG_ERROR				0x01
#define ATA_REG_FEATURES			0x01
#define ATA_REG_SECCOUNT0			0x02
#define ATA_REG_LBA0				0x03
#define ATA_REG_LBA1				0x04
#define ATA_REG_LBA2				0x05
#define ATA_REG_HDDEVSEL			0x06
#define ATA_REG_COMMAND				0x07
#define ATA_REG_STATUS				0x07
#define is_ata_command_reg(reg) (reg <= 0x07)
/* LBA48 */
#define ATA_REG_SECCOUNT1			0x02
#define ATA_REG_LBA3				0x03
#define ATA_REG_LBA4				0x04
#define ATA_REG_LBA5				0x05

/* BAR1/3 */
#define ATA_CONTROL_REG_BASE		0x0C
#define ATA_REG_CONTROL				0x0C
#define ATA_REG_ALTSTATUS			0x0C
#define ATA_REG_DEVADDRESS			0x0D
#define is_ata_control_reg(reg) (reg >= 0x08 && reg <= 0x0f)

/* Bits */
#define ATA_BIT_NIEN				0x02
#define ATA_BIT_SRST				0x04
#define ATA_BIT_CTL_OBS				0x08
#define ATA_BIT_HOB					0x80
#define ATA_CAP_BIT_DMA				(1 << 8)
#define ATA_CAP_BIT_LBA				(1 << 9)
#define ATA_FCS5_BIT_LBA48			(1 << 10)
#define ATA_SEL_BIT_DEV				0xA0
#define ATA_SEL_BIT_LBA				(1 << 6)
#define ATA_SEL_BIT_SLAVE			(1 << 4)

/* Signatures. */
#define ATAPI_LBA1_SIG				0x14
#define ATAPI_LBA2_SIG				0xeb
#define SATAPI_LBA1_SIG				0x69
#define SATAPI_LBA2_SIG				0x96

/* ATA/IDE channel declarations. */

/* Standard PC IO port offsets for IDE channels. */
#define IDE_PRI_COMMAND				0x01F0
#define IDE_PRI_CONTROL				0x03F6
#define IDE_SEC_COMMAND				0x0170
#define IDE_SEC_CONTROL				0x0376

/* Standard PC interrupts for IDE channels. */
#define IDE_PRI_IRQ					14
#define IDE_SEC_IRQ					15

/* ide_drive constants */
#define IDE_CNL_PRIMARY				0
#define IDE_CNL_SECONDARY			1
#define IDE_DRV_MASTER				0
#define IDE_DRV_SLAVE				1
#define IDE_ATA						0
#define IDE_ATAPI					1

/* Default sector size. */
#define IDE_SECTOR_SIZE				512

/* Macros */
#define ide_is_error(x) (x != IDE_ER_NOERR)

struct ide_channel
{
	struct thread_mutex mutex; /* Mutex protecting the channel. */

	uint16_t pio_command; /* Channel command port. */
	uint16_t pio_control; /* Channel control port. */

	byte buffer[2048]; /* Channel immediate buffer. */

	void *opaque; /* Driver data. */
};

struct ide_drive
{
	/* Constant part. */

	bool present; /* Is drive present? */
	uint num; /* Driver assigned drive number. */
	byte drive; /* Master or slave. */
	byte type; /* ATA or ATAPI */

	char serial[21]; /* ATA serial number info */
	char firmware[9]; /* ATA firmware info */
	char model[41]; /* ATA model info */
	uint16_t signature; /* ATA signature */
	uint16_t capabilities; /* ATA capabilities */
	uint16_t fcs1;
	uint16_t fcs2;
	uint16_t fcs3;
	uint16_t fcs4;
	uint16_t fcs5;
	uint16_t fcs6;

	uint64_t sectors; /* Total number of sectors. */

	void *opaque; /* Driver data. */

	struct ide_channel *channel; /* Parent channel. */
};

/* kernel/ata_pio.c */

void ata_pio_lock(struct ide_channel *cp);
void ata_pio_unlock(struct ide_channel *cp);

void ata_pio_register_write(struct ide_channel *cp, byte reg, byte value);
byte ata_pio_register_read(struct ide_channel *cp, byte reg);
void ata_pio_register_read_buffer(struct ide_channel *cp, byte reg, uint16_t *buffer, uint16_t words);
void ata_pio_wait_for_status(struct ide_channel *cp, byte mask, byte status);
byte ata_pio_poll(struct ide_channel *cp, bool read_status);
byte ata_pio_read(struct ide_drive *dp, uint64_t lba, uint16_t sectors, void *buffer);
byte ata_pio_write(struct ide_drive *dp, uint64_t lba, uint16_t sectors, const void *buffer);

#endif
