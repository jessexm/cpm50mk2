/*
    zdiloader.c  - Loads L92 bootstrap code using over ZDI interface.
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
#include <ez8.h>
#include "zdi.h"

/* ez80L92 loader image can be created with srec_cat
   srec_cat l92pffbl.hex -intel -o l92pffbl.h -c-array l92pffbl */
#include "l92pffbl.h"

#define SYS_CLOCK      5.5296F
#define TIMER_PRESCALE 2
#define WAIT_US(us)    wait((us * (SYS_CLOCK / TIMER_PRESCALE)) + 1)

typedef char          int8_t;
typedef unsigned char uint8_t;
typedef int           int16_t;
typedef unsigned int  uint16_t;


void zdiWriteByte(uint8_t addr, uint8_t data)
{
    uint8_t i;

    ZDI_START;

    // 7-bit address
    i = 7;
    do {
        // shift here gets rid of the unused MSB
        addr <<= 1;

        if (addr & 0x80) {
            ZDA_HIGH;
        } else {
            ZDA_LOW;
        }

        ZDI_TOGGLE;

    } while (--i);

    // read/write bit
    ZDI_WRITE;

    // single-bit separator
    ZDI_SINGLE_LOW;

    // data byte 
    i = 8;
    do {
        if (data & 0x80) {
            ZDA_HIGH;
        } else {
            ZDA_LOW;
        }

        data <<= 1;

        ZDI_TOGGLE;

    } while (--i);

    // end with ZCL and ZDA in idle state
    ZDI_IDLE;
}


void zdiWriteBlock(uint8_t addr, const uint8_t *data, uint16_t len)
{
    uint8_t i;
    uint8_t byte;
    uint16_t count;

    ZDI_START;

    // 7-bit address
    i = 7;
    do {
        // shift here gets rid of the unused MSB
        addr <<= 1;

        if (addr & 0x80) {
            ZDA_HIGH;
        } else {
            ZDA_LOW;
        }

        ZDI_TOGGLE;

    } while (--i);

    // read/write bit
    ZDI_WRITE;

    // single-bit separator
    ZDI_SINGLE_LOW;

    // data
    for (count = 0; count < len; ++count) {
        byte = data[count];

        for (i = 0; i < 8; ++i) {
            if (byte & 0x80) {
                ZDA_HIGH;
            } else {
                ZDA_LOW;
            }

            byte <<= 1;

            ZDI_TOGGLE;
        }

        // single-bit separator
        ZDI_SINGLE_LOW;
    }

    // end with ZCL and ZDA in idle state
    ZDI_IDLE;
}


void zdiReadByte(uint8_t addr, uint8_t *data)
{
    uint8_t i;

    ZDI_START;

    // 7-bit address
    i = 7;
    // do...while generates smaller code than for loop
    do {
        // shift here gets rid of the unused MSB
        addr <<= 1;

        if (addr & 0x80) {
            ZDA_HIGH;
        } else {
            ZDA_LOW;
        }

        ZDI_TOGGLE;

    } while (--i);

    // read/write bit
    ZDI_READ;

    // single-bit separator
    ZDI_SINGLE_HIGH;

    // change direction of ZDA to input
    PADD = ~(L92RST_PIN | ZCL_PIN);

    // data
    *data = 0;
    i = 8;
    do {
        ZCL_HIGH;

        *data <<= 1;

        if (ZDA_IN & ZDA_PIN) {
            *data |= 0x01;
        }

        ZCL_LOW;
        
    } while(--i);

    // change direction of ZDA to output
    PADD = ~(L92RST_PIN | ZCL_PIN | ZDA_PIN);

    // end with ZCL and ZDA in idle state
    ZDI_IDLE;
}


void wait(uint16_t n)
{
    T0RH = n >> 8;
    T0RL = n & 0xff;

    T0CTL1 = 0x88;
    
    while (!(IRQ0 & 0x20)) {}

    IRQ0 = 0x00;
}


void main(void)
{
    uint16_t done;
    uint8_t status;

    /* After reset port A pins default to inputs. The ZDA and ZCL pins have 
       external pull-ups and L92RST has a pull-down. Before configuring these 
       pins as outputs set their state to equal the state forced by the resistors. */
    ZDA_HIGH;
    ZCL_HIGH;
    L92RST_LOW;

    /* Configure L92RST, ZCL and ZDA as outputs
       PADD assigns PAADDR to the proper PADD access value and 
       assigns PACTL the rvalue, see ez8.h */
    PADD = ~(L92RST_PIN | ZCL_PIN | ZDA_PIN);

    // It can take up to 10ms for the 50MHz clock to start up.
    WAIT_US(20000);

    /* This loop should not be required but I have seen at least one L92 
       refuse to go into a break condition after power-on-reset. So keep 
       trying until it succeeds. */
    done = 0;

    while (!done) {
        /* If ZDA is low and ZCL is high at the end of the L92s reset 
           cycle the L92 will enter the break condition on the first
           instruction after reset. */
        ZCL_HIGH;
        ZDA_LOW;

        // Release L92 from reset
        L92RST_HIGH;

        /* The L92 idles for 257 clock cycles after reset goes high.
           Wait here before trying to use the ZDI interface. */
        WAIT_US(10);

        // return ZDA high in preparation for a ZDI start signal
        ZDA_HIGH;

        // get L92 status
        zdiReadByte(ZDI_STAT, &status);

        // reset and repeat if the L92 is not under ZDI control
        if (!(status & 0x80)) {
            // hold reset low 10ms
            L92RST_LOW;
            WAIT_US(10000);
        } else {
            // L92 is in break condition and ready for ZDI commands
            done = 1;
        }
    }

    // set program counter to image load address
    zdiWriteByte(ZDI_WR_DATA_L, L92PFFBL_START & 0xFFU);
    zdiWriteByte(ZDI_WR_DATA_H, (L92PFFBL_START >> 8) & 0xFFU);
    zdiWriteByte(ZDI_WR_DATA_U, (L92PFFBL_START >> 16) & 0xFFU);
    zdiWriteByte(ZDI_RW_CTL, 0x87);

    // load L92 boot image to RAM
    zdiWriteBlock(ZDI_WR_MEM, l92pffbl, sizeof(l92pffbl));

    // force L92 reset
    zdiWriteByte(ZDI_MASTER_CTL, 0x80);

    // done
    asm(" stop");
}
