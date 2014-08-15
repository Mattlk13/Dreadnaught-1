#ifndef BOOTINFO_H
#define BOOTINFO_H

#include "lib/common.h"

struct multiboot {
	u32int m_flags;
	u64int m_memorySize;

	u32int m_bootDevice;
	u32int m_cmdLine;
	u32int m_modsCount;
	u32int m_modsAddr;
	u32int m_syms0;
	u32int m_syms1;
	u32int m_syms2;
	u32int m_mmap_length;
	u32int m_mmap_addr;
	u32int m_drives_length;
	u32int m_drives_addr;
	u32int m_config_table;
	u32int m_bootloader_name;
	u32int m_apm_table;
	u32int m_vbe_control_info;
	u32int m_vbe_mode_info;
	u16int m_vbe_mode;
	u32int m_vbe_interface_addr;
	u16int m_vbe_interface_len;
};

#endif