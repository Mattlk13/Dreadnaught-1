// loader.h -- Brad Slayter

#ifndef LOADER_H
#define LOADER_H

#include "lib/common.h"

/* Most of this is from the ELF standard. Some parts
	may have been taken from toaruos. Thank you klange! */

// ELF Magic Signature
#define ELFMAG0   0x7F
#define ELFMAG1   'E'
#define ELFMAG2   'L'
#define ELFMAG3   'F'
#define EI_NIDENT 16

// ELF Datatypes
typedef u32int Elf32_Word;
typedef u32int Elf32_Addr;
typedef u32int Elf32_Off;
typedef u32int Elf32_Sword;
typedef u32int Elf32_Half;

// ELF Header
typedef struct {
	u8int 		e_ident[EI_NIDENT];
	Elf32_Half  e_type;
	Elf32_Half	e_machine;
	Elf32_Word 	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word 	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Header;

// e_type
#define EM_NONE 0
#define EM_386	3

#define EV_NONE	   0
#define EV_CURRENT 1

// Program Header
typedef struct {
	Elf32_Word p_type;
	Elf32_Off  p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Word p_filesz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
} Elf32_Phdr;

// p_type values
#define PT_NULL    0 // unused
#define PT_LOAD    1 // loadable segment
#define PT_DYNAMIC 2 // Dynamic linking info
#define PT_INTERP  3 // interpreter (null string, pathname)
#define PT_NOTE	   4 // aux info
#define PT_SHLIB   5 // Reserved
#define PT_PHDR	   6 // back-ref to the header table
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7FFFFFFF

// Section header
typedef struct {
	Elf32_Word sh_name;
	Elf32_Word sh_type;
	Elf32_Word sh_flags;
	Elf32_Addr sh_addr;
	Elf32_Off  sh_offset;
	Elf32_Word sh_size;
	Elf32_Word sh_link;
	Elf32_Word sh_info;
	Elf32_Word sh_addralign;
	Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct {
	u32int id;
	uintptr_t ptr;
} Elf32_auxv;

#define SHT_NONE 	 0
#define SHT_PROGBITS 1
#define SHT_SVMTAB	 2
#define SHT_STRTAB	 3
#define SHT_NOBITS	 8

#endif