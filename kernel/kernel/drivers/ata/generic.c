/* kernel/drivers/ata/generic.c - generic PCI ATA driver */
#include <kernel/block.h>
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>
#include <kernel/ticks.h>
#include <kernel/utils.h>
#include <kernel/devices/ata.h>
#include <kernel/devices/pci.h>

#define GEN_ATA_CACHE_SIZE 20

/* Supported devices */

static struct pci_device_id supported[] = {
		{ 0x8086, 0x7010 }
};

/* Driver data */

static atomic_bool gen_ata_inserted = false;
#define GEN_ATA_CHANNELS_NUM 2
static struct ide_channel *gen_ata_channels;
#define GEN_ATA_DRIVES_NUM 4
static struct ide_drive *gen_ata_drives;

/* Driver interfaces */

/* pci_driver */

static void gen_ata_pci_init(struct pci_driver *driver, struct pci_function *pci);

static struct pci_driver gen_ata_pci_driver = {
		.supported = supported,
		.num_supported = sizeof(supported) / sizeof(struct pci_device_id),
		.init = gen_ata_pci_init,
		.opaque = NULL,
};

/* block_dev */

static uint gen_ata_bd_lock(struct block_dev *dev, uint num);
static void gen_ata_bd_unlock(struct block_dev *dev, uint index);
static void gen_ata_bd_write(struct block_dev *dev, uint index, uint off, const byte *src, uint len);
static void gen_ata_bd_read(struct block_dev *dev, uint index, uint off, byte *dest, uint len);

#define GEN_ATA_BLOCK_DEV_CTOR(n)	\
{									\
	.name = "ata"#n,				\
	.valid = false,					\
	.block_size = IDE_SECTOR_SIZE,	\
	.num_blocks = 0,				\
	.opaque = NULL,					\
									\
	.lock = gen_ata_bd_lock,		\
	.unlock = gen_ata_bd_unlock,	\
	.write = gen_ata_bd_write,		\
	.read = gen_ata_bd_read,		\
}

static struct block_dev gen_ata_block_devices[GEN_ATA_DRIVES_NUM] = {
	GEN_ATA_BLOCK_DEV_CTOR(0),
	GEN_ATA_BLOCK_DEV_CTOR(1),
	GEN_ATA_BLOCK_DEV_CTOR(2),
	GEN_ATA_BLOCK_DEV_CTOR(3)
};

/* pci_driver */

static void rotate_words(char *words)
{
	char c;
	int i, len = kstrlen(words);

	for (i = 0; i < len; i += 2)
	{
		c = words[i];
		words[i] = words[i + 1];
		words[i + 1] = c;
	}
}

static inline void gen_ata_init_channel(byte channel, uint16_t cmd, uint16_t ctl)
{
	struct ide_channel *cp = &(gen_ata_channels[channel]);

	thread_mutex_create(&(cp->mutex));
	cp->pio_command = cmd;
	cp->pio_control = ctl;

	ata_pio_lock(cp);
	ata_pio_register_write(cp, ATA_REG_CONTROL, 0);
	ata_pio_unlock(cp);
}

static void gen_ata_init_drive(uint drive_index, byte channel, byte drive)
{
	struct ide_channel *cp = &(gen_ata_channels[channel]);
	struct ide_drive *dp = &(gen_ata_drives[drive_index]);
	byte status;
	byte type = IDE_ATA;
	byte lba1, lba2;
	uint16_t *idbuf = (uint16_t *)cp->buffer;

	ata_pio_lock(cp);

	dp->channel = cp;
	dp->present = false; // Assuming that no drive here.
	dp->num = drive_index;

	ata_pio_wait_for_status(cp, ATA_SR_BSY | ATA_SR_DRQ, 0);

	/* Select the drive. */
	ata_pio_register_write(cp, ATA_REG_HDDEVSEL, ATA_SEL_BIT_DEV | (drive << 4));
	ticks_mwait(1);

	/* Disable interrupts for this drive. */
	ata_pio_register_write(cp, ATA_REG_CONTROL, ATA_BIT_NIEN | ATA_BIT_CTL_OBS);
	ata_pio_wait_for_status(cp, ATA_SR_BSY, 0);

	/* Send an ATA IDENTIFY command. */
	ata_pio_wait_for_status(cp, ATA_SR_BSY, 0);
	ata_pio_register_write(cp, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	ticks_mwait(1);

	/* If ALT STATUS is zero it means there's no drive. */
	if (ata_pio_register_read(cp, ATA_REG_ALTSTATUS) == 0)
		goto failed;

	/* Poll for a result. */
	while (1)
	{
		status = ata_pio_register_read(cp, ATA_REG_ALTSTATUS);
		if ((status & ATA_SR_ERR))
			/* Could be an ATAPI drive. */
			break;
		if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
			/* No error! There's a drive in here! */
			break;
	}

	if (status & ATA_SR_ERR)
	{
		/* Look for an ATAPI signature. */
		lba1 = ata_pio_register_read(cp, ATA_REG_LBA1);
		lba2 = ata_pio_register_read(cp, ATA_REG_LBA2);

		if (lba1 == ATAPI_LBA1_SIG && lba2 == ATAPI_LBA2_SIG)
			type = IDE_ATAPI;
		else if (lba1 == SATAPI_LBA1_SIG && lba2 == SATAPI_LBA2_SIG)
			type = IDE_ATAPI;
		else
			goto failed;

		/* We don't support ATAPI devices yet. */
		if (type == IDE_ATAPI)
		{
			kdprintf("Generic ATA driver: drive %d:%d is an ATAPI drive\n", channel, drive);
			goto failed;
		}

		/* Try to send an ATAPI IDENITFY command. */
		ata_pio_wait_for_status(cp, ATA_SR_BSY, 0);
		ata_pio_register_write(cp, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
		ticks_mwait(1);
	}

	dp->present = true;
	dp->drive = drive;
	dp->type = type;

	/* Read the identification space of the drive. */
	ata_pio_wait_for_status(cp, ATA_SR_DRQ, 1);
	ata_pio_register_read_buffer(cp, ATA_REG_DATA, idbuf, 256);

	ata_pio_poll(cp, true);

	/* Analyze the result. */
	dp->signature = idbuf[ATA_IDENT_DEVICETYPE];
	dp->capabilities = idbuf[ATA_IDENT_CAPABILITIES];
	dp->fcs1 = idbuf[ATA_IDENT_FCS1];
	dp->fcs2 = idbuf[ATA_IDENT_FCS2];
	dp->fcs3 = idbuf[ATA_IDENT_FCS3];
	dp->fcs4 = idbuf[ATA_IDENT_FCS4];
	dp->fcs5 = idbuf[ATA_IDENT_FCS5];
	dp->fcs6 = idbuf[ATA_IDENT_FCS6];

	/* Get the size of the drive in sectors. */
	if (dp->fcs5 & ATA_FCS5_BIT_LBA48)
		// Device uses 48-Bit Addressing:
		dp->sectors = *((uint64_t *)(idbuf + ATA_IDENT_MAX_LBA_EXT));
	else
		// Device uses CHS or 28-bit Addressing:
		dp->sectors = *((uint32_t *)(idbuf + ATA_IDENT_MAX_LBA));

	/* Read the serial number, terminate it and rotate words (ATA string). */
	kmemcpy(dp->serial, idbuf + ATA_IDENT_SERIAL, 20);
	dp->serial[20] = 0;
	rotate_words(dp->serial);

	/* Read the firmware version, terminate it and rotate words (ATA string). */
	kmemcpy(dp->firmware, idbuf + ATA_IDENT_FIRMWARE, 8);
	dp->firmware[8] = 0;
	rotate_words(dp->firmware);

	/* Read the model string, terminate it and rotate words (ATA string). */
	kmemcpy(dp->model, idbuf + ATA_IDENT_MODEL, 40);
	dp->model[40] = 0;
	rotate_words(dp->model);

	gen_ata_block_devices[drive_index].num_blocks = dp->sectors;
	gen_ata_block_devices[drive_index].opaque = dp;
	gen_ata_block_devices[drive_index].valid = true;

	goto unlock_and_return;

failed:
	kdprintf("Generic ATA driver: failed to initialize drive %d:%d\n", channel, drive);

unlock_and_return:
	ata_pio_unlock(cp);
}

static void gen_ata_init_drives()
{
	gen_ata_init_channel(IDE_CNL_PRIMARY, IDE_PRI_COMMAND, IDE_PRI_CONTROL);
	gen_ata_init_channel(IDE_CNL_SECONDARY, IDE_SEC_COMMAND, IDE_SEC_CONTROL);

	gen_ata_init_drive(0, IDE_CNL_PRIMARY, IDE_DRV_MASTER);
	gen_ata_init_drive(1, IDE_CNL_PRIMARY, IDE_DRV_SLAVE);
	gen_ata_init_drive(2, IDE_CNL_SECONDARY, IDE_DRV_MASTER);
	gen_ata_init_drive(3, IDE_CNL_SECONDARY, IDE_DRV_SLAVE);
}

static void gen_ata_pci_init(__unused struct pci_driver *driver, struct pci_function *pci)
{
	kdprintf("Generic PCI ATA driver gen_ata_pci_init(): %x:%x.%x\n", pci->bus,
		pci->device, pci->function);

	gen_ata_init_drives();
}

/* Generic ATA I/O */

static void gen_ata_read_block(struct ide_drive *dp, uint lba, byte *dest)
{
	ata_pio_lock(dp->channel);
	ata_pio_read(dp, lba, 1, dest);
	ata_pio_unlock(dp->channel);
}

static void gen_ata_write_block(struct ide_drive *dp, uint lba, const byte *src)
{
	ata_pio_lock(dp->channel);
	ata_pio_write(dp, lba, 1, src);
	ata_pio_unlock(dp->channel);
}

/* block_dev */

/* TODO: Would be nice to have an RW lock. That would make reads much quicker. */

#define B_VALID 0x01
#define B_DIRTY 0x02

struct block
{
	/* Cache bookkeeping data. */
	uint drive; /* Drive number. */
	uint num; /* Block ID. */
	uint ref; /* Reference counter. */
	uint hit; /* Hit counter. */

	/* Own block data. */
	struct thread_mutex mutex;
	bool is_valid; /* Is block valid? */
	byte data[IDE_SECTOR_SIZE]; /* Block data. */
};

struct block_cache
{
	struct cpu_spinlock lock;

	uint num_blocks;
	struct block *blocks;
};

static struct block_cache cache;

static uint gen_ata_bd_lock(struct block_dev *dev, uint num)
{
	uint i;
	struct block *b;
	struct ide_drive *dp = (struct ide_drive*)dev->opaque;
	uint min_hits, min_hits_index;

	if (dev->valid == false)
		kpanic("gen_ata_bd_lock(): invalid block device");

	if (dp->present == false)
		kpanic("gen_ata_bd_lock(): drive not present");

	cpu_spinlock_acquire(&(cache.lock));

	for (i = 0; i < cache.num_blocks; i++)
	{
		b = cache.blocks + i;

		if (b->num == num && b->drive == dp->num)
		{
			/* Cache hit. */
			b->ref++;
			b->hit++;
			cpu_spinlock_release(&(cache.lock));
			/* Wait for block to become available. */
			thread_mutex_acquire(&(b->mutex));
			return i;
		}
	}

	/* We did not find the block we were looking for. Look for an unreferenced empty block in cache
	   that has the lowest hit rate. */

	min_hits = UINT_MAX;
	min_hits_index = cache.num_blocks;

	for (i = 0; i < cache.num_blocks; i++)
	{
		b = cache.blocks + i;

		if (b->ref == 0 && b->hit < min_hits)
		{
			min_hits = b->hit;
			min_hits_index = i;
		}
	}

	/* The index will be valid if we found a block. */
	if (min_hits_index < cache.num_blocks)
	{
		b = cache.blocks + min_hits_index;

		if (b->ref == 0)
		{
			thread_mutex_create(&(b->mutex));
			kmemset(b->data, 0, IDE_SECTOR_SIZE);
			b->drive = dp->num;
			b->num = num;
			b->ref = 1;
			b->hit = 1;
			b->is_valid = false;
			cpu_spinlock_release(&(cache.lock));
			/* Wait for block to become available. */
			thread_mutex_acquire(&(b->mutex));
			return min_hits_index;
		}
	}

	kpanic("gen_ata_bd_lock(): cache is full");
}

static void gen_ata_bd_unlock(struct block_dev *dev, uint index)
{
	struct block *b = cache.blocks + index;
	struct ide_drive *dp = (struct ide_drive*)dev->opaque;

	if (dev->valid == false)
		kpanic("gen_ata_bd_unlock(): invalid block device");

	if (!thread_mutex_held(&(b->mutex)))
		kpanic("gen_ata_bd_unlock(): block mutex not held");

	if (b->drive != dp->num)
		kpanic("gen_ata_bd_unlock(): drives do not match");

	thread_mutex_release(&(b->mutex));

	cpu_spinlock_acquire(&(cache.lock));
	b->ref--;
	cpu_spinlock_release(&(cache.lock));
}

static void gen_ata_bd_write(struct block_dev *dev, uint index, uint off, const byte *src, uint len)
{
	struct block *b = cache.blocks + index;
	struct ide_drive *dp = (struct ide_drive*)dev->opaque;

	if (dev->valid == false)
		kpanic("gen_ata_bd_write(): invalid block device");

	if (!thread_mutex_held(&(b->mutex)))
		kpanic("gen_ata_bd_write(): block mutex not held");

	if (b->drive != dp->num)
		kpanic("gen_ata_bd_write(): drives do not match");

	if (len == 0)
		return;

	if (b->is_valid == false)
	{
		gen_ata_read_block(dp, b->num, b->data);
		b->is_valid = true;
	}

	kassert(off < IDE_SECTOR_SIZE);
	kassert(off + len <= IDE_SECTOR_SIZE);

	kmemcpy(b->data + off, src, len);

	gen_ata_write_block(dp, b->num, b->data);
}

static void gen_ata_bd_read(struct block_dev *dev, uint index, uint off, byte *dest, uint len)
{
	struct block *b = cache.blocks + index;
	struct ide_drive *dp = (struct ide_drive*)dev->opaque;

	if (dev->valid == false)
		kpanic("gen_ata_bd_read(): invalid block device");

	if (!thread_mutex_held(&(b->mutex)))
		kpanic("gen_ata_bd_read(): block mutex not held");

	if (b->drive != dp->num)
		kpanic("gen_ata_bd_read(): drives do not match");

	if (len == 0)
		return;

	if (b->is_valid == false)
	{
		gen_ata_read_block(dp, b->num, b->data);
		b->is_valid = true;
	}

	kassert(off < IDE_SECTOR_SIZE);
	kassert(off + len <= IDE_SECTOR_SIZE);

	kmemcpy(dest, b->data + off, len);
}

void ata_gen_install(void)
{
	uint i;
	struct block_dev *bdev;

	if (atomic_exchange(&gen_ata_inserted, true))
		kpanic("ata_gen_install(): called twice");

	gen_ata_channels = kalloc(HEAP_NORMAL, 1, sizeof(struct ide_channel) * GEN_ATA_CHANNELS_NUM);
	gen_ata_drives = kalloc(HEAP_NORMAL, 1, sizeof(struct ide_drive) * GEN_ATA_DRIVES_NUM);

	cpu_spinlock_create(&(cache.lock), "ATA generic driver cache spinlock");
	cache.num_blocks = GEN_ATA_CACHE_SIZE;
	cache.blocks = kalloc(HEAP_NORMAL, 1, sizeof(struct block) * GEN_ATA_CACHE_SIZE);
	kmemset(cache.blocks, 0, sizeof(struct block) * GEN_ATA_CACHE_SIZE);

	pci_register_driver(&gen_ata_pci_driver);

	/* Register block devices created for found drives. */
	for (i = 0; i < GEN_ATA_DRIVES_NUM; i++)
	{
		bdev = &(gen_ata_block_devices[i]);

		if (bdev->valid)
			bdev_add(bdev);
	}
}
