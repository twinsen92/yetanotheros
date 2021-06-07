/* kernel/devices/ata_pio.c - ATA port I/O implementation */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>
#include <kernel/devices/ata.h>
#include <arch/kernel/portio.h>

void ata_pio_lock(struct ide_channel *cp)
{
	thread_mutex_acquire(&(cp->mutex));
}

void ata_pio_unlock(struct ide_channel *cp)
{
	thread_mutex_release(&(cp->mutex));
}

void ata_pio_register_write(struct ide_channel *cp, byte reg, byte value)
{
	kassert(thread_mutex_held(&(cp->mutex)));

	if (is_ata_command_reg(reg))
	{
		pio_outb(cp->pio_command + reg, value);
	}
	else if (is_ata_control_reg(reg))
	{
		pio_outb(cp->pio_control + reg - ATA_CONTROL_REG_BASE, value);
	}
}

byte ata_pio_register_read(struct ide_channel *cp, byte reg)
{
	kassert(thread_mutex_held(&(cp->mutex)));

	if (is_ata_command_reg(reg))
	{
		return pio_inb(cp->pio_command + reg);
	}
	else if (is_ata_control_reg(reg))
	{
		return pio_inb(cp->pio_control + reg - ATA_CONTROL_REG_BASE);
	}

	return 0;
}

void ata_pio_register_read_buffer(struct ide_channel *cp, byte reg, uint16_t *buffer, uint16_t words)
{
	int i;

	kassert(thread_mutex_held(&(cp->mutex)));

	for (i = 0; i < words; i++)
	{
		if (is_ata_command_reg(reg))
		{
			buffer[i] = pio_inw(cp->pio_command + reg);
		}
		else if (is_ata_control_reg(reg))
		{
			buffer[i] = pio_inw(cp->pio_control + reg - ATA_CONTROL_REG_BASE);
		}
	}
}

void ata_pio_wait_for_status(struct ide_channel *cp, byte mask, byte status)
{
	kassert(thread_mutex_held(&(cp->mutex)));

	while ((ata_pio_register_read(cp, ATA_REG_STATUS) & mask) != status * mask);
}

byte ata_pio_poll(struct ide_channel *cp, bool read_status)
{
	byte status;

	kassert(thread_mutex_held(&(cp->mutex)));

	/* Wait a little bit for BSY to be set. */
	ticks_mwait(1);

	/* Wait a for BSY to be cleared. */
	ata_pio_wait_for_status(cp, ATA_SR_BSY, 0);

	if (read_status)
	{
		status = ata_pio_register_read(cp, ATA_REG_STATUS); // Read Status Register.

		if ((status & ATA_SR_ERR) || (status & ATA_SR_DF) || (status & ATA_SR_DRQ) == 0)
			return status;
	}

	return ATA_ER_NOERR;
}

/* Disk I/O */

byte ata_pio_read(struct ide_drive *dp, uint64_t lba, uint16_t sectors, void *buffer)
{
	struct ide_channel *cp = dp->channel;
	byte selection = ATA_SEL_BIT_DEV;
	byte command = 0;
	byte flush_command = 0;
	byte err = ATA_ER_NOERR;
	uint16_t isector;

	/* Drive should be present. */
	if (dp->present == false)
		return ATA_ER_UNSUP;

	/* We will need LBA support. */
	if ((dp->capabilities & ATA_CAP_BIT_LBA) == 0)
		return ATA_ER_UNSUP;

	/* We currently don't support ATAPI devices here. TODO: Implement. */
	if (dp->type == IDE_ATAPI)
		return ATA_ER_UNSUP;

	kassert(thread_mutex_held(&(cp->mutex)));

	/* Wait until the device is not busy. */
	ata_pio_wait_for_status(cp, ATA_SR_BSY | ATA_SR_DRQ, 0);

	/* Select the drive. */
	if (dp->drive == IDE_DRV_SLAVE)
		selection |= ATA_SEL_BIT_SLAVE;

	/* We will be using LBA. */
	selection |= ATA_SEL_BIT_LBA;

	if (lba >= (1 << 28) || lba + sectors >= (1 << 28) || sectors >= (1 << 8))
	{
		ata_pio_register_write(cp, ATA_REG_HDDEVSEL, selection);
		ticks_mwait(1);

		ata_pio_register_write(cp, ATA_REG_CONTROL, ATA_BIT_HOB | ATA_BIT_NIEN | ATA_BIT_CTL_OBS);
		ticks_mwait(1);
		ata_pio_wait_for_status(cp, ATA_SR_BSY, 0);

		/* We will need LBA48 support to access above 128 GiB. */
		kassert(dp->fcs5 & ATA_FCS5_BIT_LBA48);

		ata_pio_register_write(cp, ATA_REG_SECCOUNT1, sectors >> 8);
		ata_pio_register_write(cp, ATA_REG_LBA3, lba >> 24);
		ata_pio_register_write(cp, ATA_REG_LBA4, lba >> 32);
		ata_pio_register_write(cp, ATA_REG_LBA5, lba >> 40);

		command = ATA_CMD_READ_PIO_EXT;
		flush_command = ATA_CMD_CACHE_FLUSH_EXT;
	}
	else
	{
		ata_pio_register_write(cp, ATA_REG_HDDEVSEL, selection | ((uint8_t)(lba >> 24) & 0x0f));
		ticks_mwait(1);

		ata_pio_register_write(cp, ATA_REG_CONTROL, ATA_BIT_NIEN | ATA_BIT_CTL_OBS);
		ata_pio_wait_for_status(cp, ATA_SR_BSY, 0);

		/* We can get by with LBA28. */
		command = ATA_CMD_READ_PIO;
		flush_command = ATA_CMD_CACHE_FLUSH;
	}

	/* Write parameters. */
	ata_pio_register_write(cp, ATA_REG_SECCOUNT0, sectors);
	ata_pio_register_write(cp, ATA_REG_LBA0, lba);
	ata_pio_register_write(cp, ATA_REG_LBA1, lba >> 8);
	ata_pio_register_write(cp, ATA_REG_LBA2, lba >> 16);

	/* Wait for the device to stop being busy and send the read command. */
	ata_pio_wait_for_status(cp, ATA_SR_BSY, 0);
	ata_pio_register_write(cp, ATA_REG_COMMAND, command);

	/* Read sectors. */
	for (isector = 0; isector < sectors; isector++)
	{
		err = ata_pio_poll(cp, true);

		if (err != ATA_ER_NOERR)
			return err;

		/* Wait for the device to let us read data. */
		ata_pio_wait_for_status(cp, ATA_SR_DRQ, 1);

		ata_pio_register_read_buffer(cp, ATA_REG_DATA,
			(uint16_t*) buffer + isector * IDE_SECTOR_SIZE, IDE_SECTOR_SIZE / 2);
	}

	/* Wait for the device to stop being busy and send flush command. */
	ata_pio_wait_for_status(cp, ATA_SR_BSY, 0);
	ata_pio_register_write(cp, ATA_REG_COMMAND, flush_command);
	ata_pio_poll(cp, false);

	return ATA_ER_NOERR;
}

byte ata_pio_write(__unused struct ide_drive *dp, __unused uint64_t lba, __unused uint16_t sectors,
	__unused const void *buffer)
{
	return ATA_ER_UNSUP;
}
