// fat12.h -- Brad Slayter

#ifndef FAT12_H
#define FAT12_H

#include "lib/common.h"

typedef struct _BIOS_PARAMATER_BLOCK {

	u8int			OEMName[8];
	u16int		BytesPerSector;
	u8int			SectorsPerCluster;
	u16int		ReservedSectors;
	u8int			NumberOfFats;
	u16int		NumDirEntries;
	u16int		NumSectors;
	u8int			Media;
	u16int		SectorsPerFat;
	u16int		SectorsPerTrack;
	u16int		HeadsPerCyl;
	u32int		HiddenSectors;
	u32int		LongSectors;

}BIOSPARAMATERBLOCK, *PBIOSPARAMATERBLOCK;

typedef struct _BIOS_PARAMATER_BLOCK_EXT {

	u32int			SectorsPerFat32;   //sectors per FAT
	u16int			Flags;             //flags
	u16int			Version;           //version
	u32int			RootCluster;       //starting root directory
	u16int			InfoCluster;
	u16int			BackupBoot;        //location of bootsector copy
	u16int			Reserved[6];

}BIOSPARAMATERBLOCKEXT, *PBIOSPARAMATERBLOCKEXT;

typedef struct _BOOT_SECTOR {
	u8int ignore[3];
	BIOSPARAMETERBLOCK bpb;
	BIOSPARAMETERBLOCKEXT bpbext;
	u8int filler[448];
} BOOTSECTOR, *PBOOTSECTOR;

#endif