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

FILESYSYEM fSysExt2;

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

static unsigned int set_block_number(ext2_fs_t *this, ext2_inodetable_t *inode, unsigned int iblock, unsigned int rblock) {
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

static FILE *finddir_ext2(FILE *node, char *name) {
	ext2_fs_t *this = (ext2_fs_t *)node->device;

	ext2_inodetable_t *inode = read_inode(this, node->inode);
	ASSERT(inode->mode &  EXT2_S_IFDIR);
	u8int block[this->block_size];
	ext2_dir_t *direntry = NULL;
	u8int block_nr = 0;
	inode_read_block(this, inode, node->inode, block_nr, block);
	u32int dir_offset = 0;
	u32int total_offset = 0;

	while (total_offset < inode->size) {
		if (dir_offset >= this->block_size) {
			block_nr++;
			dir_offset -= this->block_size;
			inode_read_block(this, inode, node->inode, block_nr, block);
		}
		ext2_dir_t *d_ent = (ext2_dir_t *)((uintptr_t)block + dir_offset);

		if (strlen(name) != d_ent->name_len) {
			dir_offset += d_ent->rec_len;
			total_offset += d_ent->rec_len;

			continue;
		}

		char *dname = (char *)malloc(sizeof(char) * (d_ent->name_len + 1));
		memcpy(dname, &(d_ent->name), d_ent->name_len);
		dname[d_ent->name_len] = '\0';
		if (!strcmp(dname, name)) {
			free(dname);
			direntry = (ext2_dir_t *)malloc(d_ent->rec_len);
			memcpy(direntry, d_ent, d_ent->rec_len);
			break;
		}
		free(dname);

		dir_offset += d_ent->rec_len;
		total_offset += d_ent->rec_len;
	}
	free(inode);
	if (!direntry)
		return NULL;

	FILE *outnode = (FILE *)malloc(sizeof(FILE));
	memset(outnode, 0, sizeof(FILE));

	inode = read_inode(this, direntry->inode);

	if (!node_from_file(this, inode, direntry, outnode)) {
		kprintf(K_ERROR, "Could not allocate out node.\n");
	}

	free(direntry);
	free(inode);
	return outnode;
}

static ext2_inodetable_t *read_inode(ext2_fs_t *this, u32int inode) {
	u32int group = inode / this->inodes_per_group;
	if (group > BGDS) {
		return NULL;
	}

	u32int inode_table_block = BGD[group].inode_table;
	inode -= group * this->inodes_per_group;
	u32int block_offset = ((inode - 1) * SB->inode_size) / this->block_size;
	u32int offset_in_block = (inode - 1) - block_offset * (this->block_size / SB->inode_size);

	u8int buf[this->block_size];
	ext2_inodetable_t *inodet = (ext2_inodetable_t *)malloc(SB->inode_size);

	read_block(this, inode_table_block + block_offset, buf);
	ext2_inodetable_t *inodes = (ext2_inodetable_t *)buf;

	memcpy(inodet, (u8int *)((u32int)inodes + offset_in_block * SB->inode_size), SB->inode_size);

	return inodet;
}

static u32int read_ext2(FILE *node, u32int offset, u32int size, u8int *buffer) {
	ext2_fs_t *this = node->device;
	ext2_inodetable_t *inode = read_inode(this, node->inode);

	u32int end;
	if (offset + size > inode->size) {
		end = inode->size;
	} else {
		end = offset + size;
	}

	u32int start_block = offset / this->block_size;
	u32int end_block = end / this->block_size;
	u32int end_size = end - end_block * this->block_size;
	u32int size_to_read = end - offset;
	if (end_size == 0)
		end_block--;

	if (start_block == end_block) {
		u8int buf[this->block_size];
		inode_read_block(this, inode, node->inode, start_block, buf);
		memcpy(buffer, (u8int *)(((u32int)buf) + (offset % this->block_size)), size_to_read);
		free(inode);
		return size_to_read;
	} else {
		u32int block_offset;
		u32int blocks_read = 0;
		u8int buf[this->block_size];
		for (block_offset = start_block; block_offset < end_block; block_offset++, blocks_read++) {
			if (block_offset == start_block) {
				inode_read_block(this, inode, node->inode, block_offset, buf);
				memcpy(buffer, (u8int *)(((u32int)buf) + (offset % this->block_size)), this->block_size);
			} else {
				inode_read_block(this, inode, node->inode, block_offset, buf);
				memcpy(buffer + this->block_size * blocks_read - (offset % this->block_size), buf, end_size);
			}
		}
		inode_read_block(this, inode, node->inode, end_block, buf);
		memcpy(buffer + this->block_size * blocks_read - (offset % this->block_size), buf, end_size);
	}
	free(inode);
	return size_to_read;
}

static void open_ext2(FILE *node, unsigned int flags) {

}

static void close_ext2(FILE *node) {

}

static u32int node_from_file(ext2_fs_t *this, ext2_inodetable_t *inode, ext2_dir_t *direntry, FILE *fnode) {
	if (!fnode) {
		return 0;
	}

	fnode->device = (void *)this;
	fnode->inode = direntry->inode;
	memcpy(&fnode->name, &direntry->name, direntry->name_len);
	fnode->name[direntry->name_len] = '\0';

	fnode->fileLength = inode->size;

	return 1;
}

static u32int ext2_root(ext2_fs_t *this, ext2_inodetable_t *inode, FILE *fnode) {
	if (!fnode)
		return 0;

	fnode->device = (void *)this;
	fnode->inode = 2;
	fnode->name[0] = '/';
	fnode->name[1] = '\0';

	fnode->length = inode->size;

	return 1;
}

static struct ata_device ata_secondary_slave  = {.io_base = 0x170, .control = 0x376, .slave = 1};

static void mount_ext2() {
	ext2_fs_t *this = (ext2_fs_t *)malloc(sizeof(ext2_fs_t));

	memset(this, 0x00, sizeof(ext2_fs_t));

	this->block_device = ata_secondary_slave;
	this->block_size = 1024;

	SB = malloc(this->block_size);

	kprintf(K_INFO, "Reading superblock...");
	read_block(this, 1, (u8int *)SB);
	if (SB->magic != EXT2_SUPER_MAGIC) {
		kprintf(K_ERROR, "Not a valid ext2 filesystem\n");
		return;
	}

	if (SB->inode_size == 0) {
		SB->inode_size = 128;
	}
	this->block_size = 1024 << SB->log_block_size;
	this->cache_entries = 10240;
	if (this->block_size > 2048) {
		this->cache_entries /= 4;
	}
	this->pointers_per_block = this->block_size / 4;
	BGDS = SB->blocks_count / SB->blocks_per_group;
	if (SB->blocks_per_group * BGDS < SB->blocks_count)
		BGDS += 1;

	this->inodes_per_group = SB->inodes_count / BGDS;

	DC = malloc(sizeof(ext2_disk_cache_entry_t) * this->cache_entries);
	for (u32int i = 0; i < this->cache_entries; i++) {
		DC[i].block_no = 0;
		DC[i].dirty = 0;
		DC[i].last_use = 0;
		DC[i].block = malloc(this->block_size);
	}

	int bgd_block_span = sizeof(ext2_bgdescriptor_t) * BGDS / this->block_size + 1;
	BGD = malloc(this->block_size * bgd_block_span);

	int bgd_offset = 2;

	if (this->block_size > 1024)
		bgd_offset = 1;

	for (int i = 0; i < bgd_block_span; i++) {
		read_block(this, bgd_offset + i, (u8int *)((u32int)BGD + this->block_size * i));
	}

	ext2_inodetable_t *root_inode = read_inode(this, 2);
	RN = (FILE *)malloc(sizeof(FILE));
	if (!ext2_root(this, root_inode, RN)) {
		kprintf(K_ERROR, "Could not get root node\n");
		return;
	}

	kprintf(K_OK, "Mounted EXT2 disk.\n");
	return;
}

void ext2_initialize() {
	strcpy(fSysExt2.name, "EXT2");
	fSysExt2.directory = direntry_ext2;
	fSysExt2.mount = mount_ext2;
	fSysExt2.open = open_ext2;
	fSysExt2.close = close_ext2;
	fSysExt2.read = read_ext2;
	fSysExt2.write = NULL;

	vol_register_file_system(&fSysExt2, 1);

	mount_ext2();
}