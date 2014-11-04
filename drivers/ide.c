// ide.c -- Brad Slayter

#include "drivers/ide.h"

#include "lib/stdio.h"
#include "lib/timer.h"
#include "lib/string.h"

static char drive_char = 'a';

#define ATA_TIMEOUT 300

struct ata_device {
	int io_base;
	int control;
	int slave;
	ata_identify_t identity;
};

static size_t ata_max_offset(struct ata_device *dev) {
	size_t sectors = dev->identity.sectors_48;
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
	for (int i = 0; i < 39; i += 2) {
		u8int tmp = ptr[i+1];
		ptr[i+1] = ptr[i];
		ptr[i] = tmp;
	}

	dev->identity.sectors_28 = info[6];

	kprintf(K_INFO, "Deivce name: %s\n", dev->identity.model);
	kprintf(K_NONE, "\tSectors (48): %d\n", (u32int)dev->identity.sectors_48);
	kprintf(K_NONE, "\tSectors (24): %d\n", dev->identity.sectors_28);
}

static struct ata_device ata_primary_master   = {.io_base = 0x1F0, .control = 0x3F6, .slave = 0};
static struct ata_device ata_primary_slave	  = {.io_base = 0x1F0, .control = 0x3F6, .slave = 1};
static struct ata_device ata_secondary_master = {.io_base = 0x170, .control = 0x376, .slave = 0};
static struct ata_device ata_secondary_slave  = {.io_base = 0x170, .control = 0x376, .slave = 1};

void ide_install() {
	ata_device_detect(&ata_primary_master);
	ata_device_detect(&ata_primary_slave);
	ata_device_detect(&ata_secondary_master);
	ata_device_detect(&ata_secondary_slave);
}