/*
    zdi.h  - ZDI defines and macros
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
#ifndef _ZDI_H_
#define _ZDI_H_

#define ZCL_OUT    PAOUT
#define ZCL_PIN    0x10 
#define ZDA_OUT    PAOUT
#define ZDA_IN     PAIN
#define ZDA_PIN    0x20 
#define L92RST_OUT PAOUT
#define L92RST_PIN 0x08

#define ZCL_HIGH ZCL_OUT |= ZCL_PIN
#define ZCL_LOW  ZCL_OUT &= ~ZCL_PIN

#define ZDA_HIGH ZDA_OUT |= ZDA_PIN
#define ZDA_LOW  ZDA_OUT &= ~ZDA_PIN

#define L92RST_HIGH L92RST_OUT |= L92RST_PIN
#define L92RST_LOW  L92RST_OUT &= ~L92RST_PIN

#define ZDI_START       ZDA_LOW; \
                        ZCL_LOW

#define ZDI_TOGGLE      ZCL_HIGH; \
                        ZCL_LOW

#define ZDI_READ        ZDA_HIGH; \
                        ZDI_TOGGLE

#define ZDI_WRITE       ZDA_LOW; \
                        ZDI_TOGGLE

#define ZDI_SINGLE_LOW  ZDA_LOW; \
                        ZDI_TOGGLE

#define ZDI_SINGLE_HIGH ZDA_HIGH; \
                        ZDI_TOGGLE

#define ZDI_IDLE        ZDA_HIGH; \
                        ZCL_HIGH

// ZDI Write-Only Registers
#define ZDI_ADDR0_L    0x00
#define ZDI_ADDR0_H    0x01
#define ZDI_ADDR0_U    0x02
#define ZDI_ADDR1_L    0x04
#define ZDI_ADDR1_H    0x05
#define ZDI_ADDR1_U    0x06
#define ZDI_ADDR2_L    0x08
#define ZDI_ADDR2_H    0x09
#define ZDI_ADDR2_U    0x0A
#define ZDI_ADDR3_L    0x0C
#define ZDI_ADDR3_H    0x0D
#define ZDI_ADDR3_U    0x0E
#define ZDI_BRK_CTL    0x10
#define ZDI_MASTER_CTL 0x11
#define ZDI_WR_DATA_L  0x13
#define ZDI_WR_DATA_H  0x14
#define ZDI_WR_DATA_U  0x15
#define ZDI_RW_CTL     0x16
#define ZDI_BUS_CTL    0x17
#define ZDI_IS4        0x21
#define ZDI_IS3        0x22
#define ZDI_IS2        0x23
#define ZDI_IS1        0x24
#define ZDI_IS0        0x25
#define ZDI_WR_MEM     0x30

// ZDI Read-Only Registers
#define ZDI_ID_L     0x00
#define ZDI_ID_H     0x01
#define ZDI_ID_REV   0x02
#define ZDI_STAT     0x03
#define ZDI_RD_L     0x10
#define ZDI_RD_H     0x11
#define ZDI_RD_U     0x12
#define ZDI_BUS_STAT 0x17
#define ZDI_RD_MEM   0x20

#endif
