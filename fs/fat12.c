// fat12.c -- Brad Slayter

#include "fs/fat12.h"

#include "lib/string.h"
#include "lib/stdio.h"

#include "drivers/Floppy.h"

#include "mm/physmem.h"

// Bytes per sector
#define SECTOR_SIZE 512

// FAT filesystem
FILESYSTEM fSysFat;

MOUNT_INFO mount_info;

// File Alloctaion Table (a.k.a FAT)
u8int FAT[SECTOR_SIZE*2];

// convert filename to dos format
void to_dos_file_name(const char *filename, char *fname, u32int FNameLength) {
	u32int i = 0;

	if (FNameLength > 12)
		return;

	if (!fname || !filename)
		return;

	memset(fname, 32, FNameLength);

	// 8.3 filename
	for (i = 0; i < strlen(filename)-1 && i < FNameLength; i++) {
		if (filename[i] == '.' || i == 8)
			break;

		fname[i] = toupper(filename[i]);
	}

	// add extension if needed
	if (filename[i] == '.') {
		for (int k = 0; k < 3; k++) {
			i++;
			if (filename[i]) {
				fname[8+k] = toupper(filename[i]);
			}
		}
	}

	fname[10] = ' ';
}

FILE fsys_fat_directory(const char *directoryName) {
	FILE file;
   	unsigned char *buf;
   	PDIRECTORY directory;

   	char dosFileName[12];
   	to_dos_file_name(directoryName, dosFileName, 12);
   	dosFileName[11] = 0; // null terminate

   	for (int sector = 0; sector < 1; sector++) {
      	// read sector
      	buf = (unsigned char *)flpy_read_sector(19 + sector);

      	directory = (PDIRECTORY)buf;

      	// 16 entries per sector
      	for (int i = 0; i < 16; i++) {
         	// get current filename
         	char name[12];
         	memcpy(name, directory->filename, 11);
         	name[11] = 0;

         	// does it match?
         	if (!strcmp(dosFileName, name)) {
				// found it
				strcpy(file.name, directoryName);
				file.id = 0;
				file.currentCluster = directory->firstCluster;
				file.eof = 0;
				file.fileLength = directory->fileSize;

				// set file type
				if (directory->attrib == 0x10)
					file.flags = FS_DIRECTORY;
				else
					file.flags = FS_FILE;

				return file;
			} else {
				//kprintf(K_ERROR, "strcmp() returned: %d", ret);
			}

			directory++;
			//getch();
		}
	}

	kprintf(K_ERROR, "Literally could not find file\n");
	// unable to find file
	file.flags = FS_INVALID;
	return file;
}

void fsys_fat_read(PFILE file, unsigned char *buffer, u32int length) {
	if (file) {
		// starting physical sector
		u32int physSector = 50 + (file->currentCluster - 1);

		unsigned char *sector = (unsigned char *)flpy_read_sector(physSector);

		// copy block of memory
		memcpy(buffer, sector, 512);

		u32int FAT_Offset = file->currentCluster + (file->currentCluster / 2);
		u32int FAT_Sector = 1 + (FAT_Offset / SECTOR_SIZE);
		u32int entryOffset = FAT_Offset %  SECTOR_SIZE;

		// read 1st FAT sector
		sector = (unsigned char *)flpy_read_sector(FAT_Sector);
		memcpy(FAT, sector, 512);

		// read 2nd FAT sector
		sector = (unsigned char *)flpy_read_sector(FAT_Sector+1);
		memcpy(FAT+SECTOR_SIZE, sector, 512);

		// read entry for next cluster
		u16int nextCluster = *(u16int *)&FAT[entryOffset];

		if (file->currentCluster & 0x0001)
			nextCluster >>= 4;
		else
			nextCluster &= 0x0FFF;

		// test for EOF
		if (nextCluster >= 0xFF8) {
			file->eof = 1;
			return;
		}

		// set next cluster
		file->currentCluster = nextCluster;
	}
}

void fsys_fat_close(PFILE file) {
	if (file)
		file->flags = FS_INVALID;
}

FILE fsys_fat_open_subdir(FILE kFile, const char *filename) {
	FILE file;

	// get 8.3 dir name
	char dosFileName[11];
	to_dos_file_name(filename, dosFileName, 11);
	dosFileName[11] = 0; // null terminate

	while (!kFile.eof) {
		unsigned char buf[512];
		fsys_fat_read(&file, buf, 512);

		PDIRECTORY pkDir = (PDIRECTORY)buf;

		for (u32int i = 0; i < 16; i++) {
			char name[11];
			memcpy(name, pkDir->filename, 11);
			name[11] = 0;

			if (!strcmp(dosFileName, name)) {
				// found it
				strcpy(file.name, filename);
				file.id = 0;
				file.currentCluster = pkDir->firstCluster;
				file.fileLength = pkDir->fileSize;
				file.eof = 0;

				if (pkDir->attrib == 0x10)
					file.flags = FS_DIRECTORY;
				else
					file.flags = FS_FILE;

				return file;
			}

			pkDir++;
		}
	}

	file.flags = FS_INVALID;
	return file;
}

FILE fsys_fat_open(const char *filename) {
	//kprintf(K_INFO, "FAT12 open file\n");
	FILE curDirectory;
	char *p = 0;
	u8int rootDir = 0;
	char *path = (char *)filename;

	// TODO: write strchr() to detect char
	if (!p) {
		curDirectory = fsys_fat_directory(path);

		if (curDirectory.flags == FS_FILE) {
			// found it
			return curDirectory;
		}

		kprintf(K_ERROR, "File found was not of type file\n");
		FILE ret;
		ret.flags = FS_INVALID;
		return ret;
	}

	kprintf(K_ERROR, "File not found from fsys_fat_directory()\n");
	curDirectory.flags = FS_INVALID;
	return curDirectory;
}

void fsys_fat_mount() {
	PBOOTSECTOR bootsector;
	bootsector = (PBOOTSECTOR)flpy_read_sector(0); // read boot sector

	kprintf(K_INFO, "Num FATs in disk %d\n", bootsector->bpb.NumberOfFats);
	kprintf(K_INFO, "Num sectors per FAT %d\n", bootsector->bpb.SectorsPerFat);


	// store mount info
	mount_info.numSectors 	  = bootsector->bpb.NumSectors;
	mount_info.fatOffset 	  = 1;
	mount_info.fatSize 		  = bootsector->bpb.SectorsPerFat;
	mount_info.fatEntrySize   = 8;
	mount_info.numRootEntries = bootsector->bpb.NumDirEntries;
	mount_info.rootOffset 	  = (bootsector->bpb.NumberOfFats * bootsector->bpb.SectorsPerFat) + 1;
	mount_info.rootSize 	  = (bootsector->bpb.NumDirEntries * 32) / bootsector->bpb.BytesPerSector;
}

void fsys_fat_initialize() {
	strcpy(fSysFat.name, "FAT12");
	fSysFat.directory = fsys_fat_directory;
	fSysFat.mount = fsys_fat_mount;
	fSysFat.open = fsys_fat_open;
	fSysFat.read = fsys_fat_read;
	fSysFat.close = fsys_fat_close;

	vol_register_file_system(&fSysFat, 0);

	fsys_fat_mount();
}