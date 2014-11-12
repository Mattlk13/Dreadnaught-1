SOURCES := crtbegin.o Calculator.o syscall.o

CC=i686-elf-gcc

CFLAGS=-nostdlib -fno-builtin -fno-stack-protector -lgcc -std=gnu99
#CFLAGS=-nostdlib -ffreestanding -O2 -lgcc -std=gnu99 -Iinclude
LDFLAGS=-T link.ld -ffreestanding -O2 -nostdlib
ASFLAGS=-felf

all: $(SOURCES) link

clean:
	-rm *.o Calc.dre

link:
	i686-elf-gcc $(LDFLAGS) -o Calc.dre $(SOURCES)

.s.o:
	nasm $(ASFLAGS) $<