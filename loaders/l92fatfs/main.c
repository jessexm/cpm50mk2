/*
    main.c  - eZ80L92 SD/FAT Disk Manager for CP/M 50 Mk II

    This module is loaded by l92pffbl; the cold start bootloader.
    The hex file l92fatfs.hex should be converted to binary and
    placed in the SD cards root directory as L92FFBL.BIN
    Note the filename must be uppercase as the Petit FAT Filesystem
    used by l92pffbl does not understand anything else.

    SRecord can be used for the hex to binary conversion. An example
    command line is:
    srec_cat l92fatfs.hex -intel -offset -0x10000 -o L92FFBL.BIN -binary

    Note the example command removes the load offset in addition to the
    format conversion.
*/
/*-
 * Copyright (c) 2018 Jesse Marroquin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <eZ80L92.h>
#include <uartdefs.h>
#include <stdio.h>
#include <stddef.h>
#include "ff.h"

#define TMR_IRQ    0x80
#define TMR_LDRST  0x02
#define TMR_ENABLE 0x01

#define SD_DR  PC_DR
#define SD_LED 0x40

#define NUM_DRIVES   6
#define BOOT_DRIVE   0

#define DRV_A_FNAME "drivea.cpm"
#define DRV_B_FNAME "driveb.cpm"
#define DRV_C_FNAME ""
#define DRV_D_FNAME ""
#define DRV_I_FNAME ""
#define DRV_J_FNAME ""

#define MSIZE 62
#define BIAS  ((MSIZE - 20) * 1024)
#define CPP   (13312 + BIAS)
#define BDOS  (CPP + 2054)
#define BIOS  (CPP + 5632)

#define JMP_VECTOR_CNT  17
#define JMP_VECTOR_SIZE 3
#define DPH_SIZE 16

#define DPHBASE (BIOS + (JMP_VECTOR_SIZE * JMP_VECTOR_CNT))
#define DPBBASE (DPHBASE + (DPH_SIZE * NUM_DISKS))

#define FLOPPY_SPT 26
#define HDISK_SPT  128
#define SECTOR_LEN 128

#define SYS_CLK 50000000
#define CONSOLE_UART_BPS 57600
#define CONSOLE_BRG      (unsigned short)(SYS_CLK / (CONSOLE_UART_BPS * 16))

typedef struct {
    char fname[40];
    FIL fp;
    // disk image details
    unsigned char spt;
    unsigned char sectlen;
    unsigned char resv;
} drive_t;

drive_t drive[NUM_DRIVES] = {
    { DRV_A_FNAME, { 0 }, FLOPPY_SPT, SECTOR_LEN, 2 },
    { DRV_B_FNAME, { 0 }, FLOPPY_SPT, SECTOR_LEN, 2 },
    { DRV_C_FNAME, { 0 }, FLOPPY_SPT, SECTOR_LEN, 2 },
    { DRV_D_FNAME, { 0 }, FLOPPY_SPT, SECTOR_LEN, 2 },
    { DRV_I_FNAME, { 0 }, HDISK_SPT, SECTOR_LEN, 0 },
    { DRV_J_FNAME, { 0 }, HDISK_SPT, SECTOR_LEN, 0 }
};

FATFS fs;

BYTE dskop(BYTE (*func)(), WORD dsk_op, BYTE track, BYTE sector, BYTE *buf)
{
    BYTE drvno = dsk_op & 0xFF;
    FSIZE_t offs;
    FRESULT result = 1;
    UINT cnt;

    if (drvno >= NUM_DRIVES) {
        return 1;
    }
                                        // UNITS:
    offs = track * drive[drvno].spt;    // sectors
    offs += sector - 1;                 // sectors
    offs *= drive[drvno].sectlen;       // bytes

    if (drive[drvno].fp.obj.fs != 0) {
        if (f_lseek(&drive[drvno].fp, offs) == FR_OK) {
            if (f_tell(&drive[drvno].fp) == offs) {
                if (dsk_op & 0xFF00) {
                    result = f_write(&drive[drvno].fp, buf, drive[drvno].sectlen, &cnt);
                } else {
                    result = f_read(&drive[drvno].fp, buf, drive[drvno].sectlen, &cnt);
                }
            }
        }
    }

    return !!result;
}

/* STDIO function */
int getch(void)
{
    while (!(UART0_LSR & UART_LSR_DR));
    return UART0_RBR;
}

/* STDIO function */
int putch(int c)
{
    while (!(UART0_LSR & UART_LSR_TEMT));

    UART0_THR = c;

    if (c == '\n') {
        while (!(UART0_LSR & UART_LSR_TEMT));
        UART0_THR = '\r';
    }

    return c;
}

void con_init(void)
{
    UART0_LCTL = UART_LCTL_DLAB;
    UART0_BRG_L = CONSOLE_BRG;
    UART0_BRG_H = CONSOLE_BRG >> 8;
    UART0_LCTL = UART_LCTL_8DATABITS;

    PD_DDR |= 0x03;
    PD_ALT1 &= ~0x03;
    PD_ALT2 |= 0x03;
}

#define DLY_US(n) dly_us((n * 12.5F) + 1)

void dly_us(unsigned short n)
{
    TMR0_RR_H = (n & 0xFF00) >> 8;
    TMR0_RR_L = n;

    TMR0_CTL = (TMR_LDRST | TMR_ENABLE);

    while (!(TMR0_CTL & TMR_IRQ));
}

void dly_ms(unsigned short n)
{
    do {
        DLY_US(1000);
    } while (--n);
}

void blink_halt(unsigned char n)
{
    unsigned char i;

    for (;;) {
        for (i = 0; i < n; i++) {
            SD_DR &= ~SD_LED;
            dly_ms(200);
            SD_DR |= SD_LED;
            dly_ms(200);
        }

        dly_ms(1000);
    }
}

void load_cpm(void)
{
    unsigned char nsects;
    unsigned char track;
    unsigned char sector;
    unsigned char *ptr;

    nsects = (drive[BOOT_DRIVE].spt * drive[BOOT_DRIVE].resv) - 1;
    track = 0;
    sector = 2;
    ptr = (unsigned char *)CPP;

    do {
        if (dskop(NULL, BOOT_DRIVE, track, sector, ptr)) {
            printf("sector read failed while loading CP/M\nsystem halted\n");
            blink_halt(3);
        }

        if (sector++ == drive[BOOT_DRIVE].spt) {
            sector = 1;
            track++;
        }

        ptr += SECTOR_LEN;

    } while (--nsects);
}

void go_cpm(void)
{
#pragma asm
    stmix
    jp.sis BIOS
#pragma endasm
}

void main(void)
{
    FRESULT rc;
    FILINFO fno;
    int i;

    con_init();

    printf("CP/M 50 Mk II, eZ80L92 SD/FAT Disk Manager, " __DATE__ " " __TIME__ "\n");

    if (f_mount(&fs, "", 0)) {
        printf("f_mount for SD card failed\nsystem halted\n");
        blink_halt(1);
    }

    if (f_open(&drive[BOOT_DRIVE].fp, drive[BOOT_DRIVE].fname, FA_READ | FA_WRITE)) {
        printf("failed to open \"%s\" for drive A\nsystem halted\n", drive[BOOT_DRIVE].fname);
        blink_halt(2);
    }

    for (i = 1; i < NUM_DRIVES; i++) {
        if (*drive[i].fname && (f_stat(drive[i].fname, &fno) == FR_OK)) {
            if (f_open(&drive[i].fp, drive[i].fname, FA_READ | FA_WRITE)) {
                printf("failed to open \"%s\" for drive %c\n", drive[i].fname, "ABCDIJ"[i]);
            }
        }
    }

    load_cpm();

    go_cpm();

    blink_halt(5);
}
