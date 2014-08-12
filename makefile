SOURCES=kernel/boot.o kernel/kmain.o

CC=i686-elf-gcc

CFLAGS=-nostdlib -fno-builtin -fno-stack-protector -lgcc -std=gnu99
LDFLAGS=-T kernel/Link.ld -ffreestanding -O2 -nostdlib
ASFLAGS=-felf

all: $(SOURCES) link
	./scripts/buildISO.sh

clean:
	-rm */*.o heisenberg.bin

link:
	i686-elf-gcc $(LDFLAGS) -o heisenberg.bin $(SOURCES)

.s.o:
	nasm $(ASFLAGS) $<