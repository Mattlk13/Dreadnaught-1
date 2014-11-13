// ide.c -- Brad Slayter

#include "drivers/ide.h"

#include "lib/stdio.h"
#include "lib/timer.h"
#include "lib/string.h"

#include "mm/blk.h"

static char drive_char = 'a';

#define ATA_TIMEOUT 300
#define ATA_SECTOR_SIZE 512

static int ata_wait(struct ata_device *dev, int advanced);

static size_t ata_max_offset(struct ata_device *dev) {
	size_t sectors = dev->identity.sectors_48;
	if (!sectors)
		sectors = dev->identity.sectors_28;

	return sectors * ATA_SECTOR_SIZE;
}

static void ata_device_read_sector(struct ata_device *dev, u32int lba, u8int *buf) {
	u16int bus = dev->io_base;
	u8int slave = dev->slave;

	// TODO: lock
	int errors = 0;
try_again:
	outb(bus + ATA_REG_CONTROL, 0x02);

	ata_wait(dev, 0);

	outb(bus + ATA_REG_HDDEVSEL, 0xE0 | slave << 4 | (lba & 0x0F000000) >> 24);
	outb(bus + ATA_REG_FEATURES, 0x00);
	outb(bus + ATA_REG_SECCOUNT0, 1);
	outb(bus + ATA_REG_LBA0, (lba & 0x000000FF) >> 0);
	outb(bus + ATA_REG_LBA1, (lba & 0x0000FF00) >> 8);
	outb(bus + ATA_REG_LBA2, (lba & 0x00FF0000) >> 16);
	outb(bus + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

	if (ata_wait(dev, 1)) {
		kprintf(K_WARN, "Error during ATA read of lba block %d\n", lba);
		errors++;
		if (errors > 4) {
			kprintf(K_ERROR, "Too many errors trying to read. Kicking out of read.\n");
			// TODO: unlock
			return;
		}
		goto try_again;
	}

	int size = 256;
	inportsm(bus, buf, size);
	ata_wait(dev, 0);
	// TODO: unlock
}

static u32int read_ata(struct ata_device *dev, u32int offset, u32int size, u8int *buffer) {
	unsigned int start_block = offset / ATA_SECTOR_SIZE;
	unsigned int end_block = (offset + size - 1) / ATA_SECTOR_SIZE;

	unsigned int x_offset = 0;

	if (offset > ata_max_offset(dev))
		return 0;

	if (offset + size > ata_max_offset(dev)) {
		unsigned int i = ata_max_offset(dev) - offset;
		size = i;
	}

	if (offset % ATA_SECTOR_SIZE) {
		unsigned int prefix_size = (ATA_SECTOR_SIZE - (offset % ATA_SECTOR_SIZE));
		char *tmp = (char *)malloc(ATA_SECTOR_SIZE);
		ata_device_read_sector(dev, start_block, (u8int *)tmp);

		memcpy(buffer, (void *)((uintptr_t)tmp + (offset % ATA_SECTOR_SIZE)), prefix_size);
		
		free(tmp);

		x_offset += prefix_size;
		start_block++;
	}

	if ((offset + size) % ATA_SECTOR_SIZE && start_block < end_block) {
		unsigned int postfix_size = (offset + size) % ATA_SECTOR_SIZE;
		char *tmp = (char *)malloc(ATA_SECTOR_SIZE);
		ata_device_read_sector(dev, end_block, (u8int *)tmp);

		memcpy((void *)((uintptr_t)buffer + size - postfix_size), tmp, postfix_size);

		free(tmp);

		end_block--;
	}

	while (start_block <= end_block) {
		ata_device_read_sector(dev, start_block, (u8int *)((uintptr_t)buffer + x_offset));
		x_offset += ATA_SECTOR_SIZE;
		start_block++;
	}

	return size;
}

static void ata_device_write_sector(struct ata_device *dev, u32int lba, u8int *buf) {
	u16int bus = dev->io_base;
	u8int slave = dev->slave;

	// TODO: lock

	outb(bus + ATA_REG_CONTROL, 0x02);

	ata_wait(dev, 0);
	outb(bus + ATA_REG_HDDEVSEL, 0xE0 | slave << 4 | (lba & 0x0F000000) >> 24);
	ata_wait(dev, 0);

	outb(bus + ATA_REG_FEATURES, 0x00);
	outb(bus + ATA_REG_SECCOUNT0, 0x01);
	outb(bus + ATA_REG_LBA0, (lba & 0x000000FF) >>  0);
	outb(bus + ATA_REG_LBA1, (lba & 0x0000FF00) >>  8);
	outb(bus + ATA_REG_LBA2, (lba & 0x00FF0000) >> 16);
	outb(bus + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
	ata_wait(dev, 0);
	int size = ATA_SECTOR_SIZE / 2;
	outportsm(bus, buf, size);
	outb(bus + 0x07, ATA_CMD_CACHE_FLUSH);
	ata_wait(dev, 0);
	// TODO: unlock
}

static int buffer_compare(u32int *ptr1, u32int *ptr2, size_t size) {
	ASSERT(!(size % 4));
	size_t i = 0;
	while (i < size) {
		if (*ptr1 != *ptr2)
			return 1;

		ptr1++;
		ptr2++;
		i += sizeof(u32int);
	}
	return 0;
}

static void ata_device_write_sector_retry(struct ata_device *dev, u32int lba, u8int *buf) {
	u8int read_buf[ATA_SECTOR_SIZE];
	asm volatile("cli");
	do {
		ata_device_write_sector(dev, lba, buf);
		ata_device_read_sector(dev, lba, read_buf);
	} while (buffer_compare((u32int *)buf, (u32int *)read_buf, ATA_SECTOR_SIZE));
	asm volatile("sti");
}

static u32int write_ata(struct ata_device *dev, u32int offset, u32int size, u8int *buffer) {
	unsigned int start_block = offset / ATA_SECTOR_SIZE;
	unsigned int end_block = (offset + size - 1) / ATA_SECTOR_SIZE;

	unsigned int x_offset = 0;

	if (offset > ata_max_offset(dev))
		return 0;

	if (offset + size > ata_max_offset(dev)) {
		unsigned int i = ata_max_offset(dev) - offset;
		size = i;
	}

	if (offset % ATA_SECTOR_SIZE) {
		unsigned int prefix_size = (ATA_SECTOR_SIZE - (offset % ATA_SECTOR_SIZE));

		char *tmp = (char *)malloc(ATA_SECTOR_SIZE);
		ata_device_read_sector(dev, start_block, (u8int *)tmp);

		kprintf(K_INFO, "Writing first block\n");

		memcpy((void *)((uintptr_t)tmp + offset % ATA_SECTOR_SIZE), buffer, prefix_size);
		ata_device_write_sector_retry(dev, start_block, (u8int *)tmp);

		free(tmp);
		x_offset += prefix_size;
		start_block++;
	}

	if ((offset + size) % ATA_SECTOR_SIZE && start_block < end_block) {
		unsigned int postfix_size = (offset + size) % ATA_SECTOR_SIZE;

		char *tmp = (char *)malloc(ATA_SECTOR_SIZE);
		ata_device_read_sector(dev, end_block, (u8int *)tmp);

		kprintf(K_INFO, "Writing last block\n");

		memcpy(tmp, (void *)((uintptr_t)buffer + size - postfix_size), postfix_size);

		ata_device_write_sector_retry(dev, end_block, (u8int *)tmp);

		free(tmp);
		end_block--;
	}

	while (start_block <= end_block) {
		ata_device_write_sector_retry(dev, start_block, (u8int *)((uintptr_t)buffer + x_offset));
		x_offset += ATA_SECTOR_SIZE;
		start_block++;
	}

	return size;
}

static void ata_io_wait(struct ata_device *dev) {
	inb(dev->io_base + ATA_REG_ALTSTATUS);
	inb(dev->io_base + ATA_REG_ALTSTATUS);
	inb(dev->io_base + ATA_REG_ALTSTATUS);
	inb(dev->io_base + ATA_REG_ALTSTATUS);
}

static int ata_wait(struct ata_device *dev, int advanced) {
	u8int status = 0;

	ata_io_wait(dev);

	while ((status = inb(dev->io_base + ATA_REG_STATUS)) & ATA_SR_BSY);

	if (advanced) {
		status = inb(dev->io_base + ATA_REG_STATUS);
		if (status 	 & ATA_SR_ERR) 	return 1;
		if (status 	 & ATA_SR_DF)	return 1;
		if (!(status & ATA_SR_DRQ)) return 1;
	}

	return 0;
}

static u8int wait_for_controller(struct ata_device *dev, u8int mask, u8int value, unsigned long timeout) {
	u8int status;
	do {
		status = inb(dev->io_base + 7);
		sleep(1);
	} while ((status & mask) != value && --timeout);
	return timeout;
}

static void ata_soft_reset(struct ata_device *dev) {
	outb(dev->control, 0x04);
	outb(dev->control, 0x00);
}

static u8int ata_reset_controller(struct ata_device *dev) {
	outb(dev->io_base + 0x206, 0x04);
	sleep(2);

	if (!wait_for_controller(dev, 0x80, 0x80, 1))
		return 0;

	outb(dev->io_base + 0x206, 0);

	if (!wait_for_controller(dev, 0x80, 0, ATA_TIMEOUT))
		return 0;

	return 1;
}

static u8int select_device(struct ata_device *dev) {
	if ((inb(dev->io_base + 7) & (ATA_SR_BSY | ATA_SR_DRQ)))
		return 0;

	outb(dev->io_base + 6, 0xA0 | (dev->slave << 4));
	sleep(1);

	if (inb(dev->io_base + 7) & (ATA_SR_BSY | ATA_SR_DRQ))
		return 0;

	return 1;
}

static void ata_device_init(struct ata_device *dev) {
	kprintf(K_INFO, "Initializing device on bus %x\n", dev->io_base);

	outb(dev->io_base + 1, 1);
	outb(dev->control, 0);

	outb(dev->io_base + ATA_REG_HDDEVSEL, 0xA0 | dev->slave << 4);
	ata_io_wait(dev);

	int status = inb(dev->io_base + ATA_REG_COMMAND);
	kprintf(K_INFO, "Device status: %d\n", status);

	ata_wait(dev, 0);

	u16int *buf = (u16int *)&dev->identity;

	for (int i = 0; i < 256; i++) {
		buf[i] = inw(dev->io_base);
	}

	u8int *ptr = (u8int *)&dev->identity.model;
	for (int i = 0; i < 39; i += 2) {
		u8int tmp = ptr[i+1];
		ptr[i+1] = ptr[i];
		ptr[i] = tmp;
	}

	kprintf(K_INFO, "Deivce name: %s\n", dev->identity.model);
	kprintf(K_NONE, "\tSectors (48): %d\n", (u32int)dev->identity.sectors_48);
	kprintf(K_NONE, "\tSectors (24): %d\n", dev->identity.sectors_28);

	outb(dev->io_base + ATA_REG_CONTROL, 0x02);
}

static int ata_device_detect(struct ata_device *dev) {
	int i, iobase = dev->io_base;
	u8int status, cl, ch, cmd;

	outb(iobase + 2, 0xAB);
	if (inb(iobase + 2) != 0xAB) {
		// No dice
		kprintf(K_ERROR, "No dice.\n");
		return -1;
	}

	ata_reset_controller(dev);

	if (!select_device(dev))
		return -1;

	if (inb(dev->io_base + 2) == 0x01 && inb(dev->io_base + 3) == 0x01) {
		cl = inb(dev->io_base + ATA_REG_LBA1);
		ch = inb(dev->io_base + ATA_REG_LBA2);
		status = inb(dev->io_base + 7);
		if (cl == 0x14 && ch == 0xEB) {
			kprintf(K_INFO, "ATAPI on bus %x\n", dev->io_base);
			cmd = 0xA1;
		} else if (cl == 0x00 && ch == 0x00 && status != 0) {
			kprintf(K_INFO, "Not ATAPI but present on bus %x\n", dev->io_base);
			cmd = 0xEC;
		} else {
			kprintf(K_INFO, "No drive.\n");
			return -1;
		}
	}

	outb(dev->io_base + 7, cmd);
	sleep(1);

	if (!wait_for_controller(dev, ATA_SR_BSY | ATA_SR_DRQ | ATA_SR_ERR,
		ATA_SR_DRQ, ATA_TIMEOUT)) {
		kprintf(K_ERROR, "Drive actually not present\n");
		return -1;
	}

	u16int *info = (u16int *)&dev->identity;
	for (i = 0; i < 256; i++)
		info[i] = inw(dev->io_base + 0);

	u8int *ptr = (u8int *)&dev->identity.model;
	for (i = 0; i < 39; i += 2) {
		u8int tmp = ptr[i+1];
		ptr[i+1] = ptr[i];
		ptr[i] = tmp;
	}

	kprintf(K_INFO, "Deivce name: %s\n", dev->identity.model);
	kprintf(K_NONE, "\tSectors (48): %d\n", (u32int)dev->identity.sectors_48);
	kprintf(K_NONE, "\tSectors (24): %d\n", dev->identity.sectors_28);
}

static struct ata_device ata_primary_master   = {.io_base = 0x1F0, .control = 0x3F6, .slave = 0};
static struct ata_device ata_primary_slave	  = {.io_base = 0x1F0, .control = 0x3F6, .slave = 1};
static struct ata_device ata_secondary_master = {.io_base = 0x170, .control = 0x376, .slave = 0};
static struct ata_device ata_secondary_slave  = {.io_base = 0x170, .control = 0x376, .slave = 1};

struct ata_device *getHDD() {
	return &ata_primary_master;
}

void ide_install() {
	//ata_device_detect(&ata_primary_master);
	//ata_device_detect(&ata_primary_slave);
	ata_device_detect(&ata_secondary_master);
	ata_device_detect(&ata_secondary_slave);

	/*char myString[] = "Hello world!";
	char *readStr = (char *)malloc(strlen(myString)+1);
	if (write_ata(&ata_secondary_slave, 100, strlen(myString)+1, myString) == strlen(myString)+1) {
		kprintf(K_OK, "Supposedly successful write\n");

		kprintf(K_INFO, "Attempting read...\n");
		if (read_ata(&ata_secondary_slave, 100, strlen(myString)+1, readStr) == strlen(myString)+1) {
			kprintf(K_OK, "Successfully read: %s\n", readStr);
		} else {
			kprintf(K_ERROR, "Read fail\n");
		}
	} else {
		kprintf(K_ERROR, "Failed write.\n");
	}
	free(readStr);*/
}