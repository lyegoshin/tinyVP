P               = mips-linux-gnu-
#P               = /usr/local/mips-2014.11/bin/mips-sde-elf-
CC              = $(P)gcc -mips32r2 -EL -g -nostdlib
OBJCOPY         = $(P)objcopy
OBJDUMP         = $(P)objdump
CFLAGS          = -O -Wall -Werror -DPIC32MZ
LDFLAGS         = -T pic32mz.ld -e _start

PROG            = uart

all:            $(PROG).hex $(PROG).srec

$(PROG).hex:   $(PROG).c
		$(CC) $(CFLAGS) -c $<
		$(CC) $(LDFLAGS) $(PROG).o $(LIBS) -o $(PROG).elf
		$(OBJCOPY) -O ihex --change-addresses -0x2BE8000 $(PROG).elf $(PROG).hex
		$(OBJDUMP) -mmips:isa32r2 -d -S $(PROG).elf > $(PROG).dis

$(PROG).srec:
		mips-linux-gnu-objcopy -O srec uart.hex uart.srec
clean:
		rm -f *.o *.lst *~ *.elf *.hex *.dis
