/*
    main.c  - eZ80L92 SD/FAT Bootstrap Loader for CP/M 50 Mk II

    This module is placed in memory by the Z8 zdiloader.
    The hex file l92pffbl.hex should be converted to a C array
    and included in the zdiloader project.

    SRecord can be used for the hex to c-array conversion. An example
    command line is:
    srec_cat l92pffbl.hex -intel -o l92pffbl.h -c-array l92pffbl
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
#include "pff.h"

#define CARD_DETECT_FAIL 1
#define CARD_MOUNT_FAIL  2
#define FILE_OPEN_FAIL   3
#define FILE_READ_FAIL   4

#define SYS_CLK_HZ 50000000
#define CONSOLE_UART_BPS 57600
#define CONSOLE_BRG (UINT)(SYS_CLK_HZ / (CONSOLE_UART_BPS * 16))

#define SD_DR  PC_DR
#define SD_DDR PC_DDR
#define SD_LED 0x40
#define SD_CD  0x20

#define BLOCK_SIZE  512
#define LOAD_ADDR   0x010000
#define LOADER_PATH "L92FFBL.BIN"

#define DLY_US(n) delay((n * 125 / 10 / 256) & 0xFF, (n * 125 / 10) & 0xFF)
void delay(BYTE h, BYTE l);

void dly_ms(UINT n)
{
    do {
        DLY_US(1000);
    } while (--n);
}

void blink_halt(UINT n)
{
    UINT i;

    for (;;) {
        for (i = 0; i < n; i++) {
            SD_DR &= ~SD_LED;
            dly_ms(250);
            SD_DR |= SD_LED;
            dly_ms(250);
        }

        dly_ms(1000);
    }
}

void con_putc(int c)
{
    while (!(UART0_LSR & UART_LSR_TEMT)) {}
    UART0_THR = c;
}

void con_puts(const char *s)
{
    while (*s) {
        con_putc(*s++);
    }

    con_putc('\r');
    con_putc('\n');
}

void con_init(void)
{
    // init UART; n81 57600
    UART0_LCTL = UART_LCTL_DLAB;
    UART0_BRG_L = CONSOLE_BRG & 0xFF;
    UART0_BRG_H = CONSOLE_BRG >> 8;
    UART0_LCTL = UART_LCTL_8DATABITS;
    UART0_FCTL = UART_FCTL_FIFOEN;

    // init UART IO
    PD_DDR |= 0x03;
    PD_ALT1 &= ~0x03;
    PD_ALT2 |= 0x03;
}

void main (void)
{
    FATFS fs;
    UINT br;
    BYTE *addr = (BYTE *)LOAD_ADDR;

    con_init();

    con_puts("CP/M 50 Mk II, eZ80L92 SD/FAT Bootstrap Loader, " __DATE__ " " __TIME__);

    SD_DR |= SD_LED;
    SD_DDR |= SD_CD;
    SD_DDR &= ~SD_LED;

    if (SD_DR & SD_CD) {
        blink_halt(CARD_DETECT_FAIL);
    }

    if (pf_mount(&fs)) {
        blink_halt(CARD_MOUNT_FAIL);
    }

    if (pf_open(LOADER_PATH)) {
        blink_halt(FILE_OPEN_FAIL);
    }

    do {
        if (pf_read(addr, BLOCK_SIZE, &br)) {
            blink_halt(FILE_READ_FAIL);
        }
        addr += BLOCK_SIZE;
    } while (br);

    (*(void (*)())LOAD_ADDR)();
}
