SOURCES := crtbegin.o fizzbuzz.o syscall.o

CC=i686-elf-gcc

CFLAGS=-nostdlib -fno-builtin -fno-stack-protector -lgcc -std=gnu99
#CFLAGS=-nostdlib -ffreestanding -O2 -lgcc -std=gnu99 -Iinclude
LDFLAGS=-T link.ld -ffreestanding -O2 -nostdlib
ASFLAGS=-felf

all: $(SOURCES) link

clean:
	-rm *.o fizz.dre

link:
	i686-elf-gcc $(LDFLAGS) -o fizz.dre $(SOURCES)

.s.o:
	nasm $(ASFLAGS) $<