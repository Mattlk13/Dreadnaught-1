// ext2.c -- Brad Slayter

#include "fs/ext2.h"
#include "fs/vfs.h"

#include "lib/stdio.h"

#include "drivers/ide.h"

#define EXT2_BGD_BLOCK 2

#define E_SUCCESS 	0
#define E_BADBLOCK 	1
#define E_NOSPACE 	2
#define E_BADPARENT 3

typedef struct {
	ext2_superblock_t *superblock;
	ext2_bgdescriptor_t *block_groups;
	FILE *rootNode;

	struct ata_device *block_device;

	unsigned int block_size;
	unsigned int pointers_per_block;
	unsigned int inodes_per_group;
	unsigned int block_group_count;

	ext2_disk_cache_entry_t *disk_cache;
	unsigned int cache_entries;
	unsigned int cache_time;

	// LOCK
} ext2_fs_t;

#define BGDS (this->block_group_count)
#define SB   (this->superblock)
#define BGD  (this->block_groups)
#define RN   (this->root_node)
#define DC   (this->disk_cache)

#define BLOCKBIT(n) (bg_buffer[((n) >> 3)] & (1 << (((n) % 8))))
#define BLOCKBYTE(n) (bg_buffer[((n) >> 3)])
#define SETBIT(n) (1 << (((n) % 8)))

static uint32_t node_from_file(ext2_fs_t * this, ext2_inodetable_t *inode, ext2_dir_t *direntry,  FILE *fnode);
static uint32_t ext2_root(ext2_fs_t * this, ext2_inodetable_t *inode, FILE *fnode);
static ext2_inodetable_t *read_inode(ext2_fs_t * this, uint32_t inode);

static unsigned int get_cache_time(ext2_fs_t *this) {
	return this->cache_time++;
}

static int cache_flush_dirty(ext2_fs_t *this, unsigned int ent_no) {
	write_ata(this->block_device, (DC[ent_no].block_no) * this->block_size, this->block_size, (u8int *)(DC[ent_no].block));
	DC[ent_no].dirty = 0;

	return E_SUCCESS;
}

static int read_block(ext2_fs_t *this, unsigned int block_no, u8int *buf) {
	if (!block_no)
		return E_BADBLOCK;

	// TODO: lock

	if (!DC) {
		read_ata(this->block_device, block_no * this->block_size, this->block_size, (u8int *)buf);

		// TODO: unlock

		return E_SUCCESS;
	}

	int oldest = -1;
	unsigned int oldest_age = 99999999;
	for (unsigned int i = 0; i < this->cache_entries; i++) {
		if (DC[i].block_no == block_no) {
			DC[i].last_use = get_cache_time(this);

			memcpy(buf, DC[i].block, this->block_size);

			// TODO: unlock

			return E_SUCCESS;
		}
		if (DC[i].last_use < oldest_age) {
			oldest = i;
			oldest_age = DC[i].last_use;
		}
	}

	if (DC[oldest].dirty)
		cache_flush_dirty(this, oldest);

	read_ata(this->block_device, block_no * this->block_size, this->block_size, (u8int *)DC[oldest].block);

	memcpy(buf, DC[oldest].block, this->block_size);

	DC[oldest].block_no = block_no;
	DC[oldest].last_use = get_cache_time(this);
	DC[oldest].dirty = 0;

	// TODO: unlock

	return E_SUCCESS;
}

static int write_block(ext2_fs_t *this, unsigned int block_no, u8int *buf) {
	if (!block_no)
		return E_BADBLOCK;

	// TODO: Lock

	int oldest = -1;
	unsigned int oldest_age = 99999999;
	for (unsigned int i = 0; i < this->cache_entries; i++) {
		if (DC[i].block_no == block_no) {
			DC[i].last_use = get_cache_time(this);
			DC[i].dirty = 1;
			memcpy(DC[i].block, buf, this->block_size);
			// TODO: Unlock
			return E_SUCCESS;
		}
		if (DC[i].last_use < oldest_age) {
			oldest = i;
			oldest_age = DC[i].last_use;
		}
	}

	if (DC[oldest].dirty) {
		cache_flush_dirty(this, oldest);
	}

	memcpy(DC[oldest].block, buf, this->block_size);
	DC[oldest].block_no = block_no;
	DC[oldest].last_use = get_cache_time(this);
	DC[oldest].dirty = 1;

	// TODO: unlock

	return E_SUCCESS;
}