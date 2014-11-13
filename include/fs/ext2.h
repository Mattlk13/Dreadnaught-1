// ext2.h -- Brad Slayter

#ifndef EXT2_H
#define EXT2_H

#include "lib/common.h"

#define EXT2_SUPER_MAGIC 0xEF53

#define EXT2_DIRECT_BLOCKS 12

struct ext2_superblock {
	u32int inodes_count;
	u32int blocks_count;
	u32int r_blocks_count;
	u32int free_blocks_count;
	u32int free_inodes_count;
	u32int first_data_block;
	u32int log_block_size;
	u32int log_frag_size;
	u32int blocks_per_group;
	u32int frags_per_group;
	u32int inodes_per_group;
	u32int mtime;
	u32int wtime;

	u16int mnt_count;
	u16int max_mnt_count;
	u16int magic;
	u16int state;
	u16int errors;
	u16int minor_rev_level;

	u32int lastcheck;
	u32int checkinterval;
	u32int creator_os;
	u32int rev_level;

	u16int def_resuid;
	u16int def_resgid;

	u32int first_ino;
	u16int inode_size;
	u16int block_group_nr;
	u32int feature_compat;
	u32int feature_incompat;
	u32int feature_ro_compat;

	u8int uuid[16];
	u8int volume_name[16];

	u8int last_mounted[64];

	u32int algo_bitmap;

	u8int prealloc_blocks;
	u8int prealloc_dir_blocks;
	u16int _padding;

	u8int journal_uuid[16];
	u32int journal_inum;
	u32int journal_dev;
	u32int last_orphan;

	u32int hash_seed[4];
	u8int def_hash_version;
	u16int _padding_a;
	u8int _padding_b;

	u32int default_mount_options;
	u32int first_meta_bg;
	u8int _unused[760];
} __attribute__ ((packed));

typedef struct ext2_superblock ext2_superblock_t;

struct ext2_bgdescriptor {
	u32int block_bitmap;
	u32int inode_bitmap;
	u32int inode_table;
	u16int free_blocks_count;
	u16int free_inodes_count;
	u16int used_dirs_count;
	u16int pad;
	u8int reserved[12];
} __attribute__ ((packed));

typedef struct ext2_bgdescriptor ext2_bgdescriptor_t;

// File Types
#define EXT2_S_IFSOCK	0xC000
#define EXT2_S_IFLNK	0xA000
#define EXT2_S_IFREG	0x8000
#define EXT2_S_IFBLK	0x6000
#define EXT2_S_IFDIR	0x4000
#define EXT2_S_IFCHR	0x2000
#define EXT2_S_IFIFO	0x1000

// setuid, etc.
#define EXT2_S_ISUID	0x0800
#define EXT2_S_ISGID	0x0400
#define EXT2_S_ISVTX	0x0200

// rights
#define EXT2_S_IRUSR	0x0100
#define EXT2_S_IWUSR	0x0080
#define EXT2_S_IXUSR	0x0040
#define EXT2_S_IRGRP	0x0020
#define EXT2_S_IWGRP	0x0010
#define EXT2_S_IXGRP	0x0008
#define EXT2_S_IROTH	0x0004
#define EXT2_S_IWOTH	0x0002
#define EXT2_S_IXOTH	0x0001

struct ext2_inodetable {
	u16int mode;
	u16int uid;
	u32int size;
	u32int atime;
	u32int ctime;
	u32int mtime;
	u32int dtime;
	u16int gid;
	u16int links_count;
	u32int blocks;
	u32int flags;
	u32int osd1;
	u32int block[15];
	u32int generation;
	u32int file_acl;
	u32int dir_acl;
	u32int faddr;
	u8int osd2[12];
} __attribute__ ((packed));

typedef struct ext2_inodetable ext2_inodetable_t;

struct ext2_dir {
	u32int inode;
	u16int rec_len;
	u8int name_len;
	u8int file_type;
	char name;
} __attribute__ ((packed));

typedef struct ext2_dir ext2_dir_t;

typedef struct {
	u32int block_no;
	u32int last_use;
	u8int dirty;
	u8int *block;
} ext2_disk_cache_entry_t;

typedef int (*ext2_block_io_t) (void *, u32int, u8int *);

void ext2_initialize();

#endif