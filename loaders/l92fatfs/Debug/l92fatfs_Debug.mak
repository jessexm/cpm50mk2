SHELL = cmd.exe

#
# ZDS II Make File - l92fatfs project, Debug configuration
#
# Generated by: ZDS II - eZ80Acclaim! 5.3.0 (Build 17041303)
#   IDE component: d:5.3.0:17021001
#   Install Path: C:\ZiLOG\ZDSII_eZ80Acclaim!_5.3.0\
#

RM = del

# ZDS base directory
ZDSDIR = C:\ZiLOG\ZDSII_eZ80Acclaim!_5.3.0
ZDSDIR_ESCSPACE = C:\ZiLOG\ZDSII_eZ80Acclaim!_5.3.0

# ZDS bin directory
BIN = $(ZDSDIR)\bin

# ZDS include base directory
INCLUDE = C:\ZiLOG\ZDSII_eZ80Acclaim!_5.3.0\include
INCLUDE_ESCSPACE = C:\ZiLOG\ZDSII_eZ80Acclaim!_5.3.0\include

# ZTP base directory
ZTPDIR = C:\ZiLOG\ZDSII_eZ80Acclaim!_5.3.0\ZTP2.5.0
ZTPDIR_ESCSPACE = C:\ZiLOG\ZDSII_eZ80Acclaim!_5.3.0\ZTP2.5.0

# project directory
PRJDIR = C:\Development\cpm50\l92fatfs
PRJDIR_ESCSPACE = C:\Development\cpm50\l92fatfs

# intermediate files directory
WORKDIR = C:\Development\cpm50\l92fatfs\Debug
WORKDIR_ESCSPACE = C:\Development\cpm50\l92fatfs\Debug

# output files directory
OUTDIR = C:\Development\cpm50\l92fatfs\Debug\
OUTDIR_ESCSPACE = C:\Development\cpm50\l92fatfs\Debug\

# tools
CC = @"$(BIN)\eZ80cc"
AS = @"$(BIN)\eZ80asm"
LD = @"$(BIN)\eZ80link"
AR = @"$(BIN)\eZ80lib"
WEBTOC = @"$(BIN)\mkwebpage"

CFLAGS =  \
-define:_DEBUG -define:_EZ80L92 -define:_EZ80 -genprintf  \
-NOkeepasm -keeplst -NOlist -NOlistinc -NOmodsect -optsize  \
-promote -NOreduceopt  \
-stdinc:"\"..;$(INCLUDE)\std;$(INCLUDE)\zilog\""  \
-usrinc:"\"..;C:\Development\cpm50\l92fatfs\ff13b\source\""  \
-NOmultithread -NOpadbranch -debug -cpu:eZ80L92  \
-asmsw:"   \
	-define:_EZ80=1 -include:\"..;$(INCLUDE)\std;$(INCLUDE)\zilog\"  \
	-list -NOlistmac -pagelen:0 -pagewidth:80 -quiet -sdiopt -warn  \
	-debug -NOigcase -cpu:eZ80L92"

ASFLAGS =  \
-define:_EZ80=1 -include:"\"..;$(INCLUDE)\std;$(INCLUDE)\zilog\""  \
-list -NOlistmac -name -pagelen:0 -pagewidth:80 -quiet -sdiopt  \
-warn -debug -NOigcase -cpu:eZ80L92

LDFLAGS = @.\l92fatfs_Debug.linkcmd
build: l92fatfs

buildall: clean l92fatfs

relink: deltarget l92fatfs

deltarget: 
	@if exist "$(WORKDIR)\l92fatfs.lod"  \
            $(RM) "$(WORKDIR)\l92fatfs.lod"
	@if exist "$(WORKDIR)\l92fatfs.hex"  \
            $(RM) "$(WORKDIR)\l92fatfs.hex"
	@if exist "$(WORKDIR)\l92fatfs.map"  \
            $(RM) "$(WORKDIR)\l92fatfs.map"

clean: 
	@if exist "$(WORKDIR)\l92fatfs.lod"  \
            $(RM) "$(WORKDIR)\l92fatfs.lod"
	@if exist "$(WORKDIR)\l92fatfs.hex"  \
            $(RM) "$(WORKDIR)\l92fatfs.hex"
	@if exist "$(WORKDIR)\l92fatfs.map"  \
            $(RM) "$(WORKDIR)\l92fatfs.map"
	@if exist "$(WORKDIR)\startup_l92.obj"  \
            $(RM) "$(WORKDIR)\startup_l92.obj"
	@if exist "$(WORKDIR)\startup_l92.lis"  \
            $(RM) "$(WORKDIR)\startup_l92.lis"
	@if exist "$(WORKDIR)\startup_l92.lst"  \
            $(RM) "$(WORKDIR)\startup_l92.lst"
	@if exist "$(WORKDIR)\main.obj"  \
            $(RM) "$(WORKDIR)\main.obj"
	@if exist "$(WORKDIR)\main.lis"  \
            $(RM) "$(WORKDIR)\main.lis"
	@if exist "$(WORKDIR)\main.lst"  \
            $(RM) "$(WORKDIR)\main.lst"
	@if exist "$(WORKDIR)\main.src"  \
            $(RM) "$(WORKDIR)\main.src"
	@if exist "$(WORKDIR)\ffunicode.obj"  \
            $(RM) "$(WORKDIR)\ffunicode.obj"
	@if exist "$(WORKDIR)\ffunicode.lis"  \
            $(RM) "$(WORKDIR)\ffunicode.lis"
	@if exist "$(WORKDIR)\ffunicode.lst"  \
            $(RM) "$(WORKDIR)\ffunicode.lst"
	@if exist "$(WORKDIR)\ffunicode.src"  \
            $(RM) "$(WORKDIR)\ffunicode.src"
	@if exist "$(WORKDIR)\diskio.obj"  \
            $(RM) "$(WORKDIR)\diskio.obj"
	@if exist "$(WORKDIR)\diskio.lis"  \
            $(RM) "$(WORKDIR)\diskio.lis"
	@if exist "$(WORKDIR)\diskio.lst"  \
            $(RM) "$(WORKDIR)\diskio.lst"
	@if exist "$(WORKDIR)\diskio.src"  \
            $(RM) "$(WORKDIR)\diskio.src"
	@if exist "$(WORKDIR)\ff.obj"  \
            $(RM) "$(WORKDIR)\ff.obj"
	@if exist "$(WORKDIR)\ff.lis"  \
            $(RM) "$(WORKDIR)\ff.lis"
	@if exist "$(WORKDIR)\ff.lst"  \
            $(RM) "$(WORKDIR)\ff.lst"
	@if exist "$(WORKDIR)\ff.src"  \
            $(RM) "$(WORKDIR)\ff.src"
	@if exist "$(WORKDIR)\dskman.obj"  \
            $(RM) "$(WORKDIR)\dskman.obj"
	@if exist "$(WORKDIR)\dskman.lis"  \
            $(RM) "$(WORKDIR)\dskman.lis"
	@if exist "$(WORKDIR)\dskman.lst"  \
            $(RM) "$(WORKDIR)\dskman.lst"
	@if exist "$(WORKDIR)\dskman.src"  \
            $(RM) "$(WORKDIR)\dskman.src"

# pre-4.11.0 compatibility
rebuildall: buildall 

LIBS = 

OBJS =  \
            $(WORKDIR_ESCSPACE)\startup_l92.obj  \
            $(WORKDIR_ESCSPACE)\main.obj  \
            $(WORKDIR_ESCSPACE)\ffunicode.obj  \
            $(WORKDIR_ESCSPACE)\diskio.obj  \
            $(WORKDIR_ESCSPACE)\ff.obj  \
            $(WORKDIR_ESCSPACE)\dskman.obj

l92fatfs: $(OBJS)
	 $(LD) $(LDFLAGS)

$(WORKDIR_ESCSPACE)\startup_l92.obj :  \
            $(PRJDIR_ESCSPACE)\startup_l92.asm  \
            $(INCLUDE_ESCSPACE)\zilog\ez80l92.inc
	 $(AS) $(ASFLAGS) "$(PRJDIR)\startup_l92.asm"

$(WORKDIR_ESCSPACE)\main.obj :  \
            $(PRJDIR_ESCSPACE)\main.c  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ff.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ffconf.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\integer.h  \
            $(INCLUDE_ESCSPACE)\std\Format.h  \
            $(INCLUDE_ESCSPACE)\std\Stdarg.h  \
            $(INCLUDE_ESCSPACE)\std\Stddef.h  \
            $(INCLUDE_ESCSPACE)\std\Stdio.h  \
            $(INCLUDE_ESCSPACE)\zilog\eZ80L92.h  \
            $(INCLUDE_ESCSPACE)\zilog\uartdefs.h
	 $(CC) $(CFLAGS) "$(PRJDIR)\main.c"

$(WORKDIR_ESCSPACE)\ffunicode.obj :  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ffunicode.c  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ff.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ffconf.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\integer.h
	 $(CC) $(CFLAGS) "$(PRJDIR)\ff13b\source\ffunicode.c"

$(WORKDIR_ESCSPACE)\diskio.obj :  \
            $(PRJDIR_ESCSPACE)\ff13b\source\diskio.c  \
            $(PRJDIR_ESCSPACE)\ff13b\source\diskio.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\integer.h  \
            $(INCLUDE_ESCSPACE)\zilog\eZ80L92.h
	 $(CC) $(CFLAGS) "$(PRJDIR)\ff13b\source\diskio.c"

$(WORKDIR_ESCSPACE)\ff.obj :  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ff.c  \
            $(PRJDIR_ESCSPACE)\ff13b\source\diskio.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ff.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ffconf.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\integer.h  \
            $(INCLUDE_ESCSPACE)\std\Stdarg.h
	 $(CC) $(CFLAGS) "$(PRJDIR)\ff13b\source\ff.c"

$(WORKDIR_ESCSPACE)\dskman.obj :  \
            $(PRJDIR_ESCSPACE)\dskman.c  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ff.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\ffconf.h  \
            $(PRJDIR_ESCSPACE)\ff13b\source\integer.h  \
            $(INCLUDE_ESCSPACE)\std\Format.h  \
            $(INCLUDE_ESCSPACE)\std\Stdarg.h  \
            $(INCLUDE_ESCSPACE)\std\Stdio.h  \
            $(INCLUDE_ESCSPACE)\std\String.h
	 $(CC) $(CFLAGS) "$(PRJDIR)\dskman.c"
