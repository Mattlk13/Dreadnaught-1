SOURCES := kernel/boot.o kernel/kmain.o io/monitor.o lib/common.o lib/string.o kernel/gdt.o \
kernel/descriptor_tables.o kernel/interrupt.o kernel/isr.o lib/timer.o lib/stdio.o\
mm/physmem.o mm/pte.o mm/pde.o mm/virtmem.o drivers/keyboard.o kernel/cmd.o fs/vfs.o\
kernel/dma.o drivers/Floppy.o fs/fat12.o lib/syscall.o kernel/Exception.o drivers/ide.o

INC=-Iinclude

CC=i686-elf-gcc

CFLAGS=-nostdlib -fno-builtin -fno-stack-protector -lgcc -std=gnu99 -Iinclude
#CFLAGS=-nostdlib -ffreestanding -O2 -lgcc -std=gnu99 -Iinclude
LDFLAGS=-T kernel/Link.ld -ffreestanding -O2 -Linclude -nostdlib
ASFLAGS=-felf

all: $(SOURCES) link
	./scripts/buildISO.sh

clean:
	-rm */*.o heisenberg.bin

link:
	i686-elf-gcc $(LDFLAGS) -o heisenberg.bin $(SOURCES)

.s.o:
	nasm $(ASFLAGS) $<