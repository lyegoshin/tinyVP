include boards/$(BOARD)

SOURCE = main.c uart.c printf.c ic.c log.c strlen.c time.c exception.c \
	branch.c tlb.c tty.c eic.c console.c scheduler.c

GENERATED_SOURCE = pte.c ic-tables.c

maps:
	build-scripts/vz-map.py $(BOARD) $(CONFIG)

build:  $(SOURCE)
	build-scripts/vz-conf.py $(BOARD) $(CONFIG) $(PLATFORM)
	mips-mti-linux-gnu-gcc -O2 \
		-DSTACKTOP=$(vzstacktop) -Deret_page=$(eret_page) \
		-membedded-data -G4096 -mgpopt -march=mips32r2 -msoft-float \
		-EL -fno-pic -mno-abicalls -mno-shared -fomit-frame-pointer \
		-ffixed-fp -fno-builtin -I. -Iinclude \
		start.S $(SOURCE) $(GENERATED_SOURCE) end.S \
		-nostdlib -lgcc -Ttext $(vzcode) -e $(vzentry) \
		-Tdata $(vzdata)
	mips-mti-linux-gnu-objcopy -O srec -x -X \
		--change-section-address \
		.sdata=`build-scripts/vz-data-addr.py a.out` \
		--remove-section=.MIPS.abiflags \
		--remove-section=.reginfo --remove-section=.comment \
		--remove-section=.pdr --remove-section=.gnu.attributes \
		--remove-section=mdebug.abi32 --remove-section=.eh_frame\
		--remove-section=.eh_frame_hdr \
		a.out a.out.srec
	./$(CONFIG).objcopy

clean:
	rm -f a.out a.out.srec a.merged.srec *.objcopy *.o
