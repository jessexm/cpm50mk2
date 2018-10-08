#
# makefile -- make me better if you can
#

CC = gcc

all: drivea

drivea: drivea.cpm
bios: bios.s28
dskman: dskman.com


bios.s28: bios.asm cpm50mk2dsk.inc
	asez80 -o bios bios.asm
	aslink -n -s -b _code=0xF200 bios.rel

dskman.com: dskman.asm
	asez80 -o dskman $^
	aslink -n -s -b _code=0x0100 dskman.rel

	srec_cat dskman.s28 --offset -0x0100 -o $@ -binary

drivea.cpm: bios.s28 dskman.com
	srec_cat -generate 0 128 -repeat-string "CP/M 50 Mk II" \
	cpm.sys -binary -exclude 0x1600 0x1E00 -offset 128 \
	bios.s28 -fill 0x00 0xF200 0xF580 -offset -0xDB80 \
	modified_cpm2-1.dsk -binary -exclude 0x0000 0x1A00 \
	-o $@ -binary

	cpmcp $@ dskman.com 0:

mkdskdef: mkdskdef.c
	$(CC) $^ -o $@

cpm50mk2dsk.inc: mkdskdef cpm50mk2dsk.def
	./mkdskdef < cpm50mk2dsk.def > $@

clean:
	rm -f bios.s28 bios.rel
	rm -f dskman.com dskman.s28 dskman.rel
	rm -f drivea.cpm
	rm -f cpm50mk2dsk.inc
	rm -f mkdskdef
