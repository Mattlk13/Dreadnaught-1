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
	for (i = 0; i < strlen(filename) && i < FNameLength; i++) {
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
			} else {
				fname[8+k] = ' ';
			}
		}
	}
	
	if (fname[10] < 'A') // sometimes garbage gets up in there
		fname[10] = ' ';
}

FILE fsys_fat_create_file(const char *directoryName, char *dosFileName) {
	FILE file;
	unsigned char *buf;
	PDIRECTORY directory;

	for (int sector = 0; sector < 14; sector++) {
		// read sector
		buf = (unsigned char *)flpy_read_sector(19 + sector);

		directory = (PDIRECTORY)buf;

		for (int i = 0; i < 16; i++) {
			// get current filename
			char name[12];
			memcpy(name, directory->filename, 11);
			name[11] = 0;

			if (name[0] == 0 || name[0] == 0xE5) {
				// found a free entry
				strcpy(directory->filename, name);
				directory->fileSize = 0;

				// find first cluster
				u16int curCluster = 0;
				while (curCluster < 1000) {
					u32int fatOffset = curCluster + (curCluster / 2);
					u8int tableEntry = *(u8int *)&FAT[fatOffset];

					if (curCluster & 0x0001)
						tableEntry >>= 4;
					else
						tableEntry &= 0x0FFF;

					if (tableEntry == 0)
						break;

					curCluster++;
				}

				directory->firstCluster = curCluster;

				strcpy(file.name, directoryName);
				file.id = 0;
				file.currentCluster = curCluster;
				file.eof = 1;
				file.fileLength = 0;
				file.flags = FS_FILE;

				if (flpy_write_sector(sector, buf))
					kprintf(K_OK, "Wrote file %s\n", directoryName);
				else
					kprintf(K_ERROR, "Unable to write file %s\n", directoryName);

				return file;
			}
			directory++;
		}
	}

	file.flags = FS_INVALID;
	kprintf(K_ERROR, "Unable to create file %s\n", directoryName);
	return file;
}

FILE fsys_fat_directory(const char *directoryName, int flags) {
	FILE file;
   	unsigned char *buf;
   	PDIRECTORY directory;

   	char dosFileName[12];
   	to_dos_file_name(directoryName, dosFileName, 12);
   	dosFileName[11] = 0; // null terminate
   	kprintf(K_DEBUG, "dosFileName: |%s|\n", dosFileName);

   	for (int sector = 0; sector < 14; sector++) {
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
				if (directory->attrib == 0x10) {
					file.flags = FS_DIRECTORY;
					kprintf(K_DEBUG, "Got a dir\n");
				} else
					file.flags = FS_FILE;

				return file;
			}

			directory++;
		}
	}

	// unable to find file
	if (flags == F_READ)
		file.flags = FS_INVALID;
	else
		return fsys_fat_create_file(directoryName, dosFileName);

	return file;
}

void fsys_fat_read(PFILE file, unsigned char *buffer, u32int length) {
	if (file) {
		// starting physical sector
		u32int physSector = 32 + (file->currentCluster - 1);

		unsigned char *sector = (unsigned char *)flpy_read_sector(physSector);

		// copy block of memory
		memcpy(buffer, sector, 512);

		u32int FAT_Offset = file->currentCluster + (file->currentCluster / 2);
		u32int FAT_Sector = 1 + (FAT_Offset / SECTOR_SIZE);
		u32int entryOffset = FAT_Offset % SECTOR_SIZE;

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

void fsys_fat_write(PFILE file, unsigned char *buffer, u32int length) {

}

void fsys_fat_close(PFILE file) {
	if (file)
		file->flags = FS_INVALID;
}

FILE fsys_fat_open_subdir(FILE kFile, const char *filename) {
	FILE file;

	// get 8.3 dir name
	char dosFileName[12];
	to_dos_file_name(filename, dosFileName, 12);
	dosFileName[11] = 0; // null terminate
	kprintf(K_INFO, "dosFileName: |%s|\n", dosFileName);

	while (!kFile.eof) {
		unsigned char buf[512];
		fsys_fat_read(&kFile, buf, 512);

		PDIRECTORY pkDir = (PDIRECTORY)buf;

		for (u32int i = 0; i < 16; i++) {
			char name[12];
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

void fsys_fat_list() {
	FILE file;
   	unsigned char *buf;
   	PDIRECTORY directory;

   	for (int sector = 0; sector < 14; sector++) {
      	// read sector
      	buf = (unsigned char *)flpy_read_sector(19 + sector);

      	directory = (PDIRECTORY)buf;

      	for (int i = 0; i < 16; i++) {
         	// get current filename
         	char name[12];
         	memcpy(name, directory->filename, 11);
         	name[11] = 0;

         	if (name[0] == 0 || name[0] == 0xE5)
         		continue;

         	if (directory->attrib != 0xF)
         		kprintf(K_NONE, "%s\n", name);

         	directory++;
        }
    }
}

FILE fsys_fat_open(const char *filename, int flags) {
	FILE curDirectory;
	char *p = 0;
	u8int rootDir = 1;
	char *path = (char *)filename;

	p = strchr(path, '/');
	if (!p) {
		curDirectory = fsys_fat_directory(path, flags);

		if (curDirectory.flags == FS_FILE) {
			// found it
			return curDirectory;
		}

		kprintf(K_ERROR, "File found was not of type file\n");
		FILE ret;
		ret.flags = FS_INVALID;
		return ret;
	}

	p = path;
	while (p) {
		char pathname[16];
		int i = 0;
		for (i = 0; i < 16; i++) {
			if (p[i] == '/' || p[i] == '\0')
				break;

			pathname[i] = p[i];
		}
		pathname[i] = 0;

		if (rootDir) {
			curDirectory = fsys_fat_directory(pathname, flags);
			rootDir = 0;
		} else {
			curDirectory = fsys_fat_open_subdir(curDirectory, pathname);
		}

		if (curDirectory.flags == FS_INVALID)
			break;

		if (curDirectory.flags == FS_FILE)
			return curDirectory;

		p = strchr(p+1, '/');
		if (p)
			p++;
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
	fSysFat.write = fsys_fat_write;
	fSysFat.close = fsys_fat_close;
	fSysFat.list = fsys_fat_list;

	vol_register_file_system(&fSysFat, 0);

	fsys_fat_mount();
}