// blk.h -- Brad Slayter

#ifndef BLK_H
#define BLK_H

#include "lib/common.h"

typedef long Align; // To align our blocks

union header {
	struct {
		union header *next; // Pointer to next memory block
		unsigned size;		// Size of the block
	} blk;
	Align x; // Force alignment
};

typedef union header Header;

#endif