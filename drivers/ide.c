// ide.c -- Brad Slayter

#include "drivers/ide.h"

#include "lib/stdio.h"
#include "lib/timer.h"
#include "lib/string.h"

static char drive_char = 'a';

struct ata_device {
	int io_base;
	int control;
	int slave;
	ata_identify_t identity;
};

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

static void ata_soft_reset(struct ata_device *dev) {
	outb(dev->control, 0x04);
	outb(dev->control, 0x00);
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
	ata_soft_reset(dev);
	outb(dev->io_base + ATA_REG_HDDEVSEL, 0xA0 | dev->slave << 4);
	ata_io_wait(dev);

	unsigned char cl = inb(dev->io_base + ATA_REG_LBA1); // CYL_LO
	unsigned char ch = inb(dev->io_base + ATA_REG_LBA2); // CYL_HI

	kprintf(K_INFO, "Device detected: %x %x\n", cl, ch);
	if (cl == 0xFF && ch == 0xFF) {
		return 0; // No drive
	} else if (cl == 0x00 && ch == 0x00) {
		// Parallel ATA device

		char devname[64] = "/dev/hd";
		char devID[1];
		devID[0] = drive_char;
		strcat(devname, devID);
		drive_char++;
		kprintf(K_INFO, "%s ready to be mounted.\n", devname);

		ata_device_init(dev);

		return 1;
	}

	return 0;
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