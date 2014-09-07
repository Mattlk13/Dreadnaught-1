// fat12.h -- Brad Slayter

#ifndef FAT12_H
#define FAT12_H

#include "lib/common.h"
#include "fs/vfs.h"

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
	BIOSPARAMATERBLOCK bpb;
	BIOSPARAMATERBLOCKEXT bpbext;
	u8int filler[448];
} BOOTSECTOR, *PBOOTSECTOR;

// Directory entry
typedef struct _DIRECTORY {
	u8int filename[8];
	u8int ext[3];
	u8int attrib;
	u8int reserved;
	u8int timeCreatedMs;
	u16int timeCreated;
	u16int dateCreated;
	u16int dateLastAccessed;
	u16int firstClusterHiBytes;
	u16int lastModTime;
	u16int lastModDate;
	u16int firstCluster;
	u32int fileSize;
} DIRECTORY, *PDIRECTORY;

typedef struct _MOUNT_INFO {
	u32int numSectors;
	u32int fatOffset;
	u32int numRootEntries;
	u32int rootOffset;
	u32int rootSize;
	u32int fatSize;
	u32int fatEntrySize;
} MOUNT_INFO, *PMOUNT_INFO;

extern FILE fsys_fat_directory(const char *directoryName, int flags);
extern void fsys_fat_read(PFILE file, unsigned char *buffer, u32int length);
extern FILE fsys_fat_open(const char *fileName, int flags);
extern void fsys_fat_initialize();
extern void fsys_fat_mount();
extern void fsys_fat_list();

#endif