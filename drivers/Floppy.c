// Floppy.c -- Brad Slayter

#include "drivers/Floppy.h"

#include "kernel/isr.h"
#include "kernel/dma.h"

#include "lib/timer.h"
#include "lib/stdio.h"

// controller I/O Ports
enum FLPYDSK_IO {
	FLPYDSK_DOR  = 0x3F2,
	FLPYDSK_MSR  = 0x3F4,
	FLPYDSK_FIFO = 0x3F5,
	FLPYDSK_CTRL = 0x3F7
};

// bits 0-4 of command byte
enum FLPYDSK_CMD {
	FDC_CMD_READ_TRACK   = 2,
	FDC_CMD_SPECIFY	     = 3,
	FDC_CMD_CHECK_STAT   = 4,
	FDC_CMD_WRITE_SECT   = 5,
	FDC_CMD_READ_SECT    = 6,
	FDC_CMD_CALIBRATE    = 7,
	FDC_CMD_CHECK_INT  	 = 8,
	FDC_CMD_FORMAT_TRACK = 0xD,
	FDC_CMD_SEEK		 = 0xF,
};


/**
*	Additional command masks. Can be masked with above commands
*/

enum FLPYDSK_CMD_EXT {

	FDC_CMD_EXT_SKIP		=	0x20,	//00100000
	FDC_CMD_EXT_DENSITY		=	0x40,	//01000000
	FDC_CMD_EXT_MULTITRACK	=	0x80	//10000000
};

/*
**	Digital Output Register
*/

enum FLPYDSK_DOR_MASK {

	FLPYDSK_DOR_MASK_DRIVE0			=	0,	//00000000	= here for completeness sake
	FLPYDSK_DOR_MASK_DRIVE1			=	1,	//00000001
	FLPYDSK_DOR_MASK_DRIVE2			=	2,	//00000010
	FLPYDSK_DOR_MASK_DRIVE3			=	3,	//00000011
	FLPYDSK_DOR_MASK_RESET			=	4,	//00000100
	FLPYDSK_DOR_MASK_DMA			=	8,	//00001000
	FLPYDSK_DOR_MASK_DRIVE0_MOTOR	=	16,	//00010000
	FLPYDSK_DOR_MASK_DRIVE1_MOTOR	=	32,	//00100000
	FLPYDSK_DOR_MASK_DRIVE2_MOTOR	=	64,	//01000000
	FLPYDSK_DOR_MASK_DRIVE3_MOTOR	=	128	//10000000
};

/**
*	Main Status Register
*/

enum FLPYDSK_MSR_MASK {

	FLPYDSK_MSR_MASK_DRIVE1_POS_MODE	=	1,	//00000001
	FLPYDSK_MSR_MASK_DRIVE2_POS_MODE	=	2,	//00000010
	FLPYDSK_MSR_MASK_DRIVE3_POS_MODE	=	4,	//00000100
	FLPYDSK_MSR_MASK_DRIVE4_POS_MODE	=	8,	//00001000
	FLPYDSK_MSR_MASK_BUSY				=	16,	//00010000
	FLPYDSK_MSR_MASK_DMA				=	32,	//00100000
	FLPYDSK_MSR_MASK_DATAIO				=	64, //01000000
	FLPYDSK_MSR_MASK_DATAREG			=	128	//10000000
};

/**
*	Controller Status Port 0
*/

enum FLPYDSK_ST0_MASK {

	FLPYDSK_ST0_MASK_DRIVE0		=	0,		//00000000	=	for completness sake
	FLPYDSK_ST0_MASK_DRIVE1		=	1,		//00000001
	FLPYDSK_ST0_MASK_DRIVE2		=	2,		//00000010
	FLPYDSK_ST0_MASK_DRIVE3		=	3,		//00000011
	FLPYDSK_ST0_MASK_HEADACTIVE	=	4,		//00000100
	FLPYDSK_ST0_MASK_NOTREADY	=	8,		//00001000
	FLPYDSK_ST0_MASK_UNITCHECK	=	16,		//00010000
	FLPYDSK_ST0_MASK_SEEKEND	=	32,		//00100000
	FLPYDSK_ST0_MASK_INTCODE	=	64		//11000000
};

/*
** LPYDSK_ST0_MASK_INTCODE types
*/

enum FLPYDSK_ST0_INTCODE_TYP {

	FLPYDSK_ST0_TYP_NORMAL		=	0,
	FLPYDSK_ST0_TYP_ABNORMAL_ERR=	1,
	FLPYDSK_ST0_TYP_INVALID_ERR	=	2,
	FLPYDSK_ST0_TYP_NOTREADY	=	3
};

/**
*	GAP 3 sizes
*/

enum FLPYDSK_GAP3_LENGTH {

	FLPYDSK_GAP3_LENGTH_STD = 42,
	FLPYDSK_GAP3_LENGTH_5_14= 32,
	FLPYDSK_GAP3_LENGTH_3_5= 27
};

/*
**	Formula: 2^sector_number * 128, where ^ denotes "to the power of"
*/

enum FLPYDSK_SECTOR_DTL {

	FLPYDSK_SECTOR_DTL_128	=	0,
	FLPYDSK_SECTOR_DTL_256	=	1,
	FLPYDSK_SECTOR_DTL_512	=	2,
	FLPYDSK_SECTOR_DTL_1024	=	4
};

// Constants

const int FLOPPY_IRQ = 6;
const int FLPY_SECTORS_PER_TRACK = 18;
const int DMA_BUFFER = 0x1000;
const int FDC_DMA_CHANNEL = 2;

static u8int current_drive = 0;
static volatile u8int _FloppyDiskIRQ = 0;

u8int dma_initialize_floppy(u8int *buffer, unsigned length, u8int write) {
	union {
		u8int byte[4];
		unsigned long l;
	} a, c;

	a.l = (unsigned)buffer;
	c.l = (unsigned)length-1;

	// check for buffer issues
	if ((a.l >> 24) || (c.l >> 16) || (((a.l & 0xFFFF)+c.l) >> 16)) {
		return 0;
	}

	dma_reset(1);
	dma_mask_channel(FDC_DMA_CHANNEL);
	dma_reset_flipflop(1);

	dma_set_address(FDC_DMA_CHANNEL, a.byte[0], a.byte[1]);
	dma_reset_flipflop(1);

	dma_set_count(FDC_DMA_CHANNEL, c.byte[0], c.byte[1]);
	if (!write)
		dma_set_read(FDC_DMA_CHANNEL);
	else
		dma_set_write(FDC_DMA_CHANNEL);

	dma_unmask_all(1);

	return 1;
}

// Basic I/O routines
u8int flpy_read_status() {
	return inb(FLPYDSK_MSR);
}

void flpy_write_dor(u8int val) {
	outb(FLPYDSK_DOR, val);
}

void flpy_send_command(u8int cmd) {
	for (int i = 0; i < 500; i++) {
		if (flpy_read_status() & FLPYDSK_MSR_MASK_DATAREG)
			return outb(FLPYDSK_FIFO, cmd);
	}
}

u8int flpy_read_data() {
	for (int i = 0; i < 500; i++) {
		if (flpy_read_status() & FLPYDSK_MSR_MASK_DATAREG)
			return inb(FLPYDSK_FIFO);
	}

	return 0;
}

// write to configuration control Register
void flpy_write_ccr(u8int val) {
	outb(FLPYDSK_CTRL, val);
}

void flpy_wait_irq() {
	while (_FloppyDiskIRQ == 0)
		;
	_FloppyDiskIRQ = 0;
}

void i86_flpy_irq(registers_t *regs) {
	_FloppyDiskIRQ = 1;
}

void flpy_check_int(u32int *st0, u32int *cyl) {
	flpy_send_command(FDC_CMD_CHECK_INT);

	*st0 = flpy_read_data();
	*cyl = flpy_read_data();
}

void flpy_control_motor(u8int b) {
	if (current_drive > 3)
		return;

	u8int motor = 0;

	switch (current_drive) {
		case 0:
			motor = FLPYDSK_DOR_MASK_DRIVE0_MOTOR;
			break;
		case 1:
			motor = FLPYDSK_DOR_MASK_DRIVE1_MOTOR;
			break;
		case 2:
			motor = FLPYDSK_DOR_MASK_DRIVE2_MOTOR;
			break;
		case 3:
			motor = FLPYDSK_DOR_MASK_DRIVE3_MOTOR;
			break;
		default:
			break;
	}

	if (b) {
		flpy_write_dor((u8int)(current_drive | motor | FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA));
	} else {
		flpy_write_dor(FLPYDSK_DOR_MASK_RESET);
	}

	sleep(10);
}

void flpy_drive_data(u8int stepr, u8int loadt, u8int unloadt, u8int dma) {
	u8int data = 0;

	flpy_send_command(FDC_CMD_SPECIFY);
	data = ((stepr & 0xF) << 4) | (unloadt & 0xF);
	flpy_send_command(data);
	data = ((loadt << 1) | ((dma) ? 0 : 1));
	flpy_send_command(data);
}

int flpy_calibrate(u8int drive) {
	u32int st0, cyl;

	if (drive >= 4)
		return -2;

	flpy_control_motor(1); // turn on motor

	for (int i = 0; i < 10; i++) {
		flpy_send_command(FDC_CMD_CALIBRATE);
		flpy_send_command(drive);
		flpy_wait_irq();
		flpy_check_int(&st0, &cyl);

		if (!cyl) {
			flpy_control_motor(0);
			return 0;
		}
	}

	flpy_control_motor(0);
	return -1;
}

void flpy_disable_controller() {
	flpy_write_dor(0);
}

void flpy_enable_controller() {
	kprintf(K_INFO, "ENABLING\n");
	flpy_write_dor(FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
	kprintf(K_INFO, "DONE ENABLING\n");
}

void flpy_reset() {
	u32int st0, cyl;

	_FloppyDiskIRQ = 0;

	//kprintf(K_INFO, "Disable Controller\n");
	outb(FLPYDSK_DOR, 0x00);
	//sleep(500);
	//kprintf(K_INFO, "Enable Controller\n");
	outb(FLPYDSK_DOR, 0x0C);
	//kprintf(K_INFO, "DONE ENABLING\n");

	while (!_FloppyDiskIRQ);

	for (int i = 0; i < 4; i++)
		flpy_check_int(&st0, &cyl);

	flpy_write_ccr(0);

	flpy_drive_data(3, 17, 240, 1);

	flpy_calibrate(current_drive);
}

void flpy_read_sector_imp(u8int head, u8int track, u8int sector, u8int write) {
	u32int st0, cyl;

	dma_initialize_floppy((u8int *)DMA_BUFFER, 512, write);

	if (!write)	
		dma_set_read(FDC_DMA_CHANNEL);
	else
		dma_set_write(FDC_DMA_CHANNEL);

	if (!write)
		flpy_send_command(FDC_CMD_READ_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
	else
		flpy_send_command(FDC_CMD_WRITE_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
	flpy_send_command(head << 2 | current_drive);
	flpy_send_command(track);
	flpy_send_command(head);
	flpy_send_command(sector);
	flpy_send_command(FLPYDSK_SECTOR_DTL_512);
	flpy_send_command(((sector + 1) >= FLPY_SECTORS_PER_TRACK) ? FLPY_SECTORS_PER_TRACK : sector + 1);
	flpy_send_command(FLPYDSK_GAP3_LENGTH_3_5);
	flpy_send_command(0xFF);

	flpy_wait_irq();

	for (int i = 0; i < 7; i++)
		flpy_read_data();

	flpy_check_int(&st0, &cyl);
}

int flpy_seek(u8int cyl, u8int head) {
	u32int st0, cyl0;

	if (current_drive >= 4)
		return -1;

	for (int i = 0; i < 10; i++) {
		flpy_send_command(FDC_CMD_SEEK);
		flpy_send_command((head) << 2 | current_drive);
		flpy_send_command(cyl);

		flpy_wait_irq();
		flpy_check_int(&st0, &cyl0);

		if (cyl0 == cyl)
			return 0;
	}

	return -1;
}

void flpy_lba_to_chs(int lba, int *head, int *track, int *sector) {
	*head = (lba % (FLPY_SECTORS_PER_TRACK * 2)) / (FLPY_SECTORS_PER_TRACK);
	*track = lba / (FLPY_SECTORS_PER_TRACK * 2);
	*sector = lba % FLPY_SECTORS_PER_TRACK + 1;
}

void flpy_install(int irq) {
	register_interrupt_handler(IRQ6, i86_flpy_irq);

	flpy_reset();

	flpy_drive_data(13, 1, 0xF, 1);
}

void flpy_set_working_drive(u8int drive) {
	if (drive < 4)
		current_drive = drive;
}

u8int flpy_get_working_drive() {
	return current_drive;
}

u8int flpy_write_sector(int sectorLBA, u8int *data) {
	if (current_drive >= 4)
		return 0;

	int head = 0, track = 0, sector = 1;
	flpy_lba_to_chs(sectorLBA, &head, &track, &sector);

	flpy_control_motor(1);
	if (flpy_seek((u8int)track, (u8int)head) != 0)
		return 0;

	memcpy((u8int *)DMA_BUFFER, data, 512);

	flpy_read_sector_imp((u8int)head, (u8int)track, (u8int)sector, 1);
	flpy_control_motor(0);

	return 1;
}

u8int *flpy_read_sector(int sectorLBA) {
	if (current_drive >= 4)
		return 0;

	int head = 0, track = 0, sector = 1;
	flpy_lba_to_chs(sectorLBA, &head, &track, &sector);

	flpy_control_motor(1);
	if (flpy_seek((u8int)track, (u8int)head) != 0)
		return 0;

	flpy_read_sector_imp((u8int)head, (u8int)track, (u8int)sector, 0);
	flpy_control_motor(0);

	// apparently this is hackish
	return (u8int *)DMA_BUFFER;
}