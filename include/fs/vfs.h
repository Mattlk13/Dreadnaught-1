// vfs.h -- Brad Slayter

#ifndef VFS_H
#define VFS_H

#include "lib/common.h"

typedef struct _FILE {
	char name[32];
	u32int flags;
	u32int fileLength;
	u32int id;
	u32int eof;
	u32int position;
	u32int currentCluster;
	u32int deviceID;
} FILE, *PFILE;

typedef enum _FLAGS {
	F_WRITE = 0,
	F_READ = 1
} FLAGS;

typedef struct _FILE_SYSTEM {
	char name[8];
	FILE (*directory)(const char *directory_name, int flags);
	void (*mount)();
	void (*read)(PFILE file, unsigned char *buffer, u32int length);
	void (*write)(PFILE file, unsigned char *buffer, u32int length);
	void (*close)(PFILE);
	FILE (*open)(const char *fileName, int flags);
	void (*list)();
} FILESYSTEM, *PFILESYSTEM;

#define FS_FILE 0
#define FS_DIRECTORY 1
#define FS_INVALID 2

extern FILE vol_open_file(const char *fname, FLAGS flags);
extern void vol_read_file(PFILE file, unsigned char *buffer, u32int length);
extern void vol_close_file(PFILE file);
extern void vol_list_dir();
extern void vol_register_file_system(PFILESYSTEM, u32int deviceID);
extern void vol_unregister_file_system(PFILESYSTEM);
extern void vol_unregister_file_system_by_id(u32int deviceID);

#endif