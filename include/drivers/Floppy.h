// Floppy.h -- Brad Slayter

#ifndef FLOPPY_H
#define FLOPPY_H

#include "lib/common.h"

void flpy_install(int irq);

// set current working drive
void flpy_set_working_drive(u8int drive);

// get current working drive
u8int flpy_get_working_drive();

// read a sector
u8int *flpy_read_sector(int sectorLBA);

// convert LBA to CHS
void flpy_lba_to_chs(int lba, int *head, int *track, int *sector);

#endif