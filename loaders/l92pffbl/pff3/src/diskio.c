/*------------------------------------------------------------------------/
/  MMCv3/SDv1/SDv2 (in SPI mode) control module for PFF
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2014, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/
/*
    Adapted to Zilog eZ80L92 Sept 2018 by Jesse Marroquin
*/
#include "diskio.h"
#include <eZ80L92.h>

#define INIT_PORT() init_port()
#define DLY_US(n)   delay((n * 125 / 10 / 256) & 0xFF, (n * 125 / 10) & 0xFF)
#define FORWARD(d)  forward(d)

#define SD_DR  PC_DR
#define SD_DDR PC_DDR
#define SD_CS  0x10
#define SD_CD  0x20
#define SD_LED 0x40

#define SELECT()   SD_DR &= ~(SD_CS | SD_LED)
#define DESELECT() SD_DR |= (SD_CS | SD_LED)

#define TMR_IRQ    0x80
#define TMR_LDRST  0x02
#define TMR_ENABLE 0x01

/* Definitions for MMC/SDC command */
#define CMD0    (0x40+0)    /* GO_IDLE_STATE */
#define CMD1    (0x40+1)    /* SEND_OP_COND (MMC) */
#define ACMD41  (0xC0+41)   /* SEND_OP_COND (SDC) */
#define CMD8    (0x40+8)    /* SEND_IF_COND */
#define CMD16   (0x40+16)   /* SET_BLOCKLEN */
#define CMD17   (0x40+17)   /* READ_SINGLE_BLOCK */
#define CMD24   (0x40+24)   /* WRITE_BLOCK */
#define CMD55   (0x40+55)   /* APP_CMD */
#define CMD58   (0x40+58)   /* READ_OCR */

/* Card type flags (CardType) */
#define CT_MMC      0x01    /* MMC ver 3 */
#define CT_SD1      0x02    /* SD ver 1 */
#define CT_SD2      0x04    /* SD ver 2 */
#define CT_SDC      (CT_SD1|CT_SD2) /* SD */
#define CT_BLOCK    0x08    /* Block addressing */

BYTE rcv_spi(void);
void xmit_spi(BYTE data);
void skip_spi(UINT n);

static
BYTE CardType;          /* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */

static
void init_port(void)
{
    // config I/O for SPI MOSI,MISO,SCK,SS
    PB_DDR |= 0xCC;
    PB_ALT1 &= ~0xCC;
    PB_ALT2 |= 0xCC;

    // config I/O for SPI chip select, activity LED, card detect
    SD_DR |= SD_CS | SD_LED;
    SD_DDR |= SD_CD;
    SD_DDR &= ~(SD_CS | SD_LED);

    // config SPI controller; SCK = 50/6 = 8.333MHz
    SPI_BRG_H = 0x00;
    SPI_BRG_L = 0x03;
    SPI_CTL = 0x30;
}

/*-----------------------------------------------------------------------*/
/* microsecond delay using timer 0                                       */
/*-----------------------------------------------------------------------*/

void delay(BYTE h, BYTE l)
{
    TMR0_RR_H = h;
    TMR0_RR_L = l;

    TMR0_CTL = (TMR_LDRST | TMR_ENABLE);

    while (!(TMR0_CTL & TMR_IRQ));
}

static
void forward(BYTE d)
{
    (void)d;
}


/*-----------------------------------------------------------------------*/
/* Transmit a byte to the card                                           */
/*-----------------------------------------------------------------------*/

static
void xmit_spi (
    BYTE d          /* Data to be sent */
)
{
    SPI_TSR = d;
    while (!SPI_SR);
}



/*-----------------------------------------------------------------------*/
/* Receive a byte from the card                                          */
/*-----------------------------------------------------------------------*/

static
BYTE rcv_spi (void)
{
    SPI_TSR = 0xFF;
    while (!SPI_SR);
    return SPI_RBR;
}



/*-----------------------------------------------------------------------*/
/* Skip bytes on the card                                                */
/*-----------------------------------------------------------------------*/

static
void skip_spi (
    UINT n      /* Number of bytes to skip */
)
{
    do {
        SPI_TSR = 0xFF;
        while (!SPI_SR);
    } while (--n);
}


/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static
void release_spi (void)
{
    DESELECT();
    rcv_spi();
}


/*-----------------------------------------------------------------------*/
/* Send a command packet to card                                         */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (
    BYTE cmd,       /* Command byte */
    DWORD arg       /* Argument */
)
{
    BYTE n, res;

    if (cmd & 0x80) {   /* ACMD<n> is the command sequense of CMD55-CMD<n> */
        cmd &= 0x7F;
        res = send_cmd(CMD55, 0);
        if (res > 1) return res;
    }

    /* Select the card */
    DESELECT();
    rcv_spi();
    SELECT();
    rcv_spi();

    /* Send a command packet */
    xmit_spi(cmd);                  /* Start + Command index */
    xmit_spi((BYTE)(arg >> 24));    /* Argument[31..24] */
    xmit_spi((BYTE)(arg >> 16));    /* Argument[23..16] */
    xmit_spi((BYTE)(arg >> 8));     /* Argument[15..8] */
    xmit_spi((BYTE)arg);            /* Argument[7..0] */
    n = 0x01;                       /* Dummy CRC + Stop */
    if (cmd == CMD0) n = 0x95;      /* Valid CRC for CMD0(0) */
    if (cmd == CMD8) n = 0x87;      /* Valid CRC for CMD8(0x1AA) */
    xmit_spi(n);

    /* Receive a command response */
    n = 10;                             /* Wait for a valid response in timeout of 10 attempts */
    do {
        res = rcv_spi();
    } while ((res & 0x80) && --n);

    return res;         /* Return with the response value */
}



/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (void)
{
    BYTE n, cmd, ty, buf[4];
    UINT tmr;


    INIT_PORT();
    DESELECT();
    skip_spi(10);           /* Dummy clocks */

    ty = 0;
    if (send_cmd(CMD0, 0) == 1) {           /* Enter Idle state */
        if (send_cmd(CMD8, 0x1AA) == 1) {   /* SDv2 */
            for (n = 0; n < 4; n++) buf[n] = rcv_spi(); /* Get trailing return value of R7 resp */
            if (buf[2] == 0x01 && buf[3] == 0xAA) {         /* The card can work at vdd range of 2.7-3.6V */
                for (tmr = 1000; tmr; tmr--) {              /* Wait for leaving idle state (ACMD41 with HCS bit) */
                    if (send_cmd(ACMD41, 1UL << 30) == 0) break;
                    DLY_US(1000);
                }
                if (tmr && send_cmd(CMD58, 0) == 0) {       /* Check CCS bit in the OCR */
                    for (n = 0; n < 4; n++) buf[n] = rcv_spi();
                    ty = (buf[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;  /* SDv2 (HC or SC) */
                }
            }
        } else {                            /* SDv1 or MMCv3 */
            if (send_cmd(ACMD41, 0) <= 1)   {
                ty = CT_SD1; cmd = ACMD41;  /* SDv1 */
            } else {
                ty = CT_MMC; cmd = CMD1;    /* MMCv3 */
            }
            for (tmr = 1000; tmr; tmr--) {          /* Wait for leaving idle state */
                if (send_cmd(cmd, 0) == 0) break;
                DLY_US(1000);
            }
            if (!tmr || send_cmd(CMD16, 512) != 0)          /* Set R/W block length to 512 */
                ty = 0;
        }
    }
    CardType = ty;
    release_spi();

    return ty ? 0 : STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read partial sector                                                   */
/*-----------------------------------------------------------------------*/

DRESULT disk_readp (
    BYTE *buff,     /* Pointer to the read buffer (NULL:Read bytes are forwarded to the stream) */
    DWORD sector,   /* Sector number (LBA) */
    UINT offset,    /* Byte offset to read from (0..511) */
    UINT count      /* Number of bytes to read (ofs + cnt mus be <= 512) */
)
{
    DRESULT res;
    BYTE d;
    UINT bc, tmr;


    if (!(CardType & CT_BLOCK)) sector *= 512;  /* Convert to byte address if needed */

    res = RES_ERROR;
    if (send_cmd(CMD17, sector) == 0) {     /* READ_SINGLE_BLOCK */

        tmr = 1000;
        do {                            /* Wait for data packet in timeout of 100ms */
            DLY_US(100);
            d = rcv_spi();
        } while (d == 0xFF && --tmr);

        if (d == 0xFE) {                /* A data packet arrived */
            bc = 514 - offset - count;

            /* Skip leading bytes */
            if (offset) skip_spi(offset);

            /* Receive a part of the sector */
            if (buff) { /* Store data to the memory */
                do
                    *buff++ = rcv_spi();
                while (--count);
            } else {    /* Forward data to the outgoing stream */
                do {
                    d = rcv_spi();
                    FORWARD(d);
                } while (--count);
            }

            /* Skip trailing bytes and CRC */
            skip_spi(bc);

            res = RES_OK;
        }
    }

    release_spi();

    return res;
}


/*-----------------------------------------------------------------------*/
/* Write partial sector                                                  */
/*-----------------------------------------------------------------------*/
#if _USE_WRITE

DRESULT disk_writep (
    const BYTE *buff,   /* Pointer to the bytes to be written (NULL:Initiate/Finalize sector write) */
    DWORD sc            /* Number of bytes to send, Sector number (LBA) or zero */
)
{
    DRESULT res;
    UINT bc, tmr;
    static UINT wc;

    res = RES_ERROR;

    if (buff) {     /* Send data bytes */
        bc = (UINT)sc;
        while (bc && wc) {      /* Send data bytes to the card */
            xmit_spi(*buff++);
            wc--; bc--;
        }
        res = RES_OK;
    } else {
        if (sc) {   /* Initiate sector write transaction */
            if (!(CardType & CT_BLOCK)) sc *= 512;  /* Convert to byte address if needed */
            if (send_cmd(CMD24, sc) == 0) {         /* WRITE_SINGLE_BLOCK */
                xmit_spi(0xFF); xmit_spi(0xFE);     /* Data block header */
                wc = 512;                           /* Set byte counter */
                res = RES_OK;
            }
        } else {    /* Finalize sector write transaction */
            bc = wc + 2;
            while (bc--) xmit_spi(0);   /* Fill left bytes and CRC with zeros */
            if ((rcv_spi() & 0x1F) == 0x05) {   /* Receive data resp and wait for end of write process in timeout of 300ms */
                for (tmr = 10000; rcv_spi() != 0xFF && tmr; tmr--)  /* Wait for ready (max 1000ms) */
                    DLY_US(100);
                if (tmr) res = RES_OK;
            }
            release_spi();
        }
    }

    return res;
}
#endif
