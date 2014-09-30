// ide.c -- Brad Slayter

#include "drivers/ide.h"

#include "lib/stdio.h"
#include "lib/timer.h"

u8int ide_read(u8int channel) {

}

void ide_wait_til_ready(u16int base) {
	while (!(inb(base) & 0x08));
}

int ide_detect_drive(u8int ide, unsigned long drive) {
	u16int tmpword;
	u8int check;
	u16int base;
	u16int buf[256];
	u16int idx;

	switch (ide) {
		case 0:
			base = 0x1F0;
			break;
		case 1:
			base = 0x170;
			break;
		default:
			return 0; // only 2 IDE controllers
			break;
	}

	outb(base+3, 0x88); 	// write
	check = inb(base + 3);  // read back

	if (check != 0x88) {
		kprintf(K_ERROR, "The IDE controller %d does not exist.\n", ide);
		return 0;
	} else {
		kprintf(K_INFO, "IDE controller %d exists.\n", ide);
	}

	outb(base+6, 0xA0 | (drive << 4));
	kprintf(K_INFO, "Sleeping...\n");
	sleep(1);
	kprintf(K_INFO, "Done sleeping.\n");

	outb(base+7, 0xEC); // send identify
	ide_wait_til_ready(base+7);

	buf[0] = inw(base);

	if (buf[0]) { // exists
		kprintf(K_OK, "Drive %d exists.\n", ide);

		for (int idx = 1; idx < 256; idx++)
			buf[idx] = inw(base);

		if (buf[0] | 0x0040 == buf[0]) {
			kprintf(K_OK, "Hard drive\n");
		} else {
			kprintf(K_ERROR, "No Hard Drive.\n");
			return 0;
		}

		return 1;
	} else {
		kprintf(K_ERROR, "Drive does not exist.\n");
		return 0;
	}
}

void ide_install() {
	ide_detect_drive(1, 1);
}