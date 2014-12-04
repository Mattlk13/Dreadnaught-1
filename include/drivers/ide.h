// ide.h -- Brad Slayter

#ifndef IDE_H
#define IDE_H

#include "lib/common.h"

#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DF      0x20
#define ATA_SR_DSC     0x10
#define ATA_SR_DRQ     0x08
#define ATA_SR_CORR    0x04
#define ATA_SR_IDX     0x02
#define ATA_SR_ERR     0x01

#define ATA_ER_BBK      0x80
#define ATA_ER_UNC      0x40
#define ATA_ER_MC       0x20
#define ATA_ER_IDNF     0x10
#define ATA_ER_MCR      0x08
#define ATA_ER_ABRT     0x04
#define ATA_ER_TK0NF    0x02
#define ATA_ER_AMNF     0x01

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

#define ATAPI_CMD_READ       0xA8
#define ATAPI_CMD_EJECT      0x1B

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
 
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

// Channels:
#define ATA_PRIMARY      0x00
#define ATA_SECONDARY    0x01

// Directions:
#define ATA_READ      0x00
#define ATA_WRITE     0x01

typedef struct {
	u16int base;
	u16int ctrl;
	u16int bmide;
	u16int nien;
} ide_channel_regs_t;

typedef struct {
	u8int reserved;
	u8int channel;
	u8int drive;
	u16int type;
	u16int signature;
	u16int capabilities;
	u32int command_sets;
	u32int size;
	u8int model[41];
} ide_device_t;

typedef struct {
	u8int status;
	u8int chs_first_sector[3];
	u8int type;
	u8int chs_last_sector[3];
	u32int lba_first_sector;
	u32int sector_count;
} partition_t;

typedef struct {
	u16int flags;
	u16int unused1[9];
	char serial[20];
	u16int unused2[3];
	char firmware[8];
	char model[40];
	u16int sectors_per_int;
	u16int unused3;
	u16int capabilities[2];
	u16int unused4[2];
	u16int valid_ext_data;
	u16int unused5[5];
	u16int size_of_rw_mult;
	u32int sectors_28;
	u16int unused6[38];
	u64int sectors_48;
	u16int unused7[152];
} __attribute__((packed)) ata_identify_t;

typedef struct {
	u8int bootstrap[446];
	partition_t partitions[4];
	u8int signature[2];
} __attribute__((packed)) mbr_t;

struct ata_device {
	int io_base;
	int control;
	int slave;
	ata_identify_t identity;
};

void ide_install();
u32int write_ata(struct ata_device *dev, u32int offset, u32int size, u8int *buffer);
u32int read_ata(struct ata_device *dev, u32int offset, u32int size, u8int *buffer);

#endif