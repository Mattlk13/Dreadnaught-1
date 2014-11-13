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

static u32int node_from_file(ext2_fs_t * this, ext2_inodetable_t *inode, ext2_dir_t *direntry,  FILE *fnode);
static u32int ext2_root(ext2_fs_t * this, ext2_inodetable_t *inode, FILE *fnode);
static ext2_inodetable_t *read_inode(ext2_fs_t * this, u32int inode);

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

static unsigned int set_block_number(ext2_fs_t *this ext2_inodetable_t *inode, unsigned int iblock, unsigned int rblock) {
	unsigned int p = this->pointers_per_block;

	unsigned int a, b, c, d, e, f, g;

	if (iblock < EXT2_DIRECT_BLOCKS) {
		inode->block[iblock] = rblock;
		return E_SUCCESS;
	} else if (iblock < EXT2_DIRECT_BLOCKS + p) {
		u8int tmp[this->block_size];
		read_block(this, inode->block[EXT2_DIRECT_BLOCKS], (u8int *)&tmp);

		((u32int *)&tmp)[iblock - EXT2_DIRECT_BLOCKS] = rblock;
		write_block(this, inode->block[EXT2_DIRECT_BLOCKS], (u8int *)&tmp);

		return E_SUCCESS;
	} else if (iblock < EXT2_DIRECT_BLOCKS + p + p * p) {
		a = iblock - EXT2_DIRECT_BLOCKS;
		b = a - p;
		c = b / p;
		d = b - c * p;

		u8int tmp[this->block_size];
		read_block(this, inode->block[EXT2_DIRECT_BLOCKS + 1], (u8int *)&tmp);

		u32int nblock = ((u32int *)&tmp)[c];
		read_block(this, nblock, (u8int *)&tmp);

		((u32int *)&tmp)[d] = rblock;
		write_block(this, nblock, (u8int *)&tmp);

		return E_SUCCESS;
	} else if (iblock < EXT2_DIRECT_BLOCKS + p + p * p + p) {
		a = iblock - EXT2_DIRECT_BLOCKS;
		b = a - p;
		c = b - p * p;
		d = c / (p * p);
		e = c - d * p * p;
		f = e / p;
		g = e - f * p;

		u8int tmp[this->block_size];
		read_block(this, inode->block[EXT2_DIRECT_BLOCKS + 2], (u8int *)&tmp);

		u32int nblock = ((u32int *)&tmp)[d];
		read_block(this, nblock, (u8int *)&tmp);

		nblock = ((u32int *)&tmp)[f];
		read_block(this, nblock, (u8int *)&tmp);

		((u32int *)&tmp)[g] = nblock;
		write_block(this, nblock, (u8int *)&tmp);

		return E_SUCCESS;
	}

	kprintf(K_ERROR, "[EXT2] Driver tried to write to a block number that was too high (%d)\n", rblock);
	return E_BADBLOCK;
}

static unsigned int get_block_number(ext2_fs_t *this, ext2_inodetable_t *inode, unsigned int iblock) {
	unsigned int p = this->pointers_per_block;

	unsigned int a, b, c, d, e, f, g;

	if (iblock < EXT2_DIRECT_BLOCKS) {
		return inode->block[iblock];
	} else if (iblock < EXT2_DIRECT_BLOCKS + p) {
		u8int tmp[this->block_size];
		read_block(this, inode->block[EXT2_DIRECT_BLOCKS], (u8int *)&tmp);

		return ((u32int *)&tmp)[iblock - EXT2_DIRECT_BLOCKS];
	} else if (iblock < EXT2_DIRECT_BLOCKS + p + p * p) {
		a = iblock - EXT2_DIRECT_BLOCKS;
		b = a - p;
		c = b / p;
		d = b - c * p;

		u8int tmp[this->block_size];
		read_block(this, inode->block[EXT2_DIRECT_BLOCKS + 1], (u8int *)&tmp);

		u32int nblock = ((u32int *)&tmp)[c];
		read_block(this, nblock, (u8int *)&tmp);

		return ((u32int *)&tmp)[d];
	} else if (iblock < EXT2_DIRECT_BLOCKS + p + p * p + p) {
		a = iblock - EXT2_DIRECT_BLOCKS;
		b = a - p;
		c = b - p * p;
		d = c / (p * p);
		e = c - d * p * p;
		f = e / p;
		g = e - f * p;

		u8int tmp[this->block_size];
		read_block(this, inode->block[EXT2_DIRECT_BLOCKS + 2], (u8int *)&tmp);

		u32int nblock = ((u32int *)&tmp)[d];
		read_block(this, nblock, (u8int *)&tmp);

		nblock = ((u32int *)&tmp)[f];
		read_block(this, nblock, (u8int *)&tmp);

		return ((u32int *)&tmp)[g];
	}

	kprintf(K_ERROR, "[EXT2] Driver tried to read a block number that was too high (%d)\n", rblock);
	return 0;
}

static int write_inode(ext2_fs_t *this, ext2_inodetable_t *inode, u32int index) {
	u32int group = index / this->inodes_per_group;
	if (group > BGDS) {
		return E_BADBLOCK;
	}

	u32int inode_table_block = BGD[group].inode_table;
	index -= group * this->inodes_per_group;
	u32int block_offset = ((index - 1) * SB->inode_size) / this->block_size;
	u32int offset_in_block = (index - 1) - block_offset * (this->block_size / SB->inode_size);

	ext2_inodetable_t *inodet = (ext2_inodetable_t *)malloc(this->block_size);

	read_block(this, inode_table_block + block_offset, (u8int *)inodet);
	memcpy((u8int *)((u32int)inodet + offset_in_block * SB->inode_size), inode, SB->inode_size);
	write_block(this, inode_table_block + block_offset, (u8int *)inodet);
	free(inodet);

	return E_SUCCESS;
}

static int allocate_inode_block(ext2_fs_t *this, ext2_inodetable_t *inode, unsigned int inode_no, unsigned int block) {
	kprintf(K_INFO, "Allocating block #%d for inode #%d", block, inode_no);
	unsigned int block_no = 0;
	unsigned int block_offset = 0;
	unsigned int group = 0;
	u8int bg_buffer[this->block_size];

	for (unsigned int i = 0; i < BGDS; i++) {
		if (BGD[i].free_blocks_count > 0) {
			read_block(this, BGD[i].block_bitmap, (u8int *)&bg_buffer);
			while (BLOCKBIT(block_offset))
				block_offset++;

			block_no = block_offset + SB->blocks_per_group * i + 1;
			group = i;
			break;
		}
	}

	if (!block_no) {
		kprintf(K_ERROR, "[EXT2] No available blocks. Out of space.\n");
		return E_NOSPACE;
	}

	u8int b = BLOCKBYTE(block_offset);
	b |= SETBIT(block_offset);
	BLOCKBYTE(block_offset) = b;
	write_block(this, BGD[group].block_bitmap, (u8int *)&bg_buffer);

	set_block_number(this, inode, block, block_no);

	BGD[group].free_blocks_count--;
	write_block(this, inode, block, block_no);

	inode->blocks++;
	write_inode(this, inode, inode_no);

	return E_SUCCESS;
}

static unsigned int inode_read_block(ext2_fs_t *this, ext2_inodetable_t *inode, unsigned int no, unsigned int block, u8int *buf) {
	if (block >= inode->blocks) {
		memset(buf, 0x0, this->block_size);
		kprintf(K_ERROR, "[EXT2] Tried to read an invalid block. Asked for %d, but inode only has %d\n", block, inode->blocks);
		return 0;
	}

	unsigned int real_block = get_block_number(this, inode, block);
	read_block(this, real_block, buf);

	return real_block;
}

static unsigned int inode_write_block(ext2_fs_t *this, ext2_inodetable_t *inode, unsigned int inode_no, unsigned int block, u8int *buf) {
	if (block >= inode->blocks) {
		kprintf(K_WARN, "[EXT2] Attempting to write beyond the existing allocated blocks for this node\n");
	}

	while (block >= inode->blocks) {
		allocate_inode_block(this, inode, inode_no, inode->blocks);
		if (block != inode->blocks - 1) {
			unsigned int real_block = get_block_number(this, inode, inode->blocks - 1);
			u8int empty[this->block_size];
			memset(&empty, 0x00, this->block_size);
			write_block(this, real_block, (u8int *)&empty);
		}
	}

	unsigned int real_block = get_block_number(this, inode, block);
	kprintf(K_INFO, "Writing virtual block %d for inode %d maps to real block %d", block, inode_no, real_block);

	write_block(this, real_block, buf);
	return real_block;
}

static ext2_dir_t *direntry_ext2(ext2_fs_t *this, ext2_inodetable_t *inode, u32int no, u32int index) {
	u8int *block = malloc(this->block_size);
	u8int block_nr = 0;
	inode_read_block(this, inode, no, block_nr, block);
	u32int dir_offset = 0;
	u32int total_offset = 0;
	u32int dir_index = 0;

	while (total_offset < inode->size && dir_index <= index) {
		ext2_dir_t *d_ent = (ext2_dir_t *)((uintptr_t)block + dir_offset);

		if (dir_index == index) {
			ext2_dir_t *out = malloc(d_ent->rec_len);
			memcpy(out, d_ent, d_ent->rec_len);
			free(block);
			return out;
		}

		dir_offset += d_ent->rec_len;
		total_offset += d_ent->rec_len;
		dir_index++;

		if (dir_offset >= this->block_size) {
			block_nr++;
			dir_offset -= this->block_size;
			inode_read_block(this, inode, no, block_nr, block);
		}
	}

	free(block);
	return NULL;
}

