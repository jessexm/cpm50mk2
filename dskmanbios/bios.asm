;
; bios.asm - a bog standard bios with the exception of
;            eZ80 mixed memory mode disk access functions
;

;
;
; Copyright (c) 2018 Jesse Marroquin
;
; Permission to use, copy, modify, and distribute this software for any
; purpose with or without fee is hereby granted, provided that the above
; copyright notice and this permission notice appear in all copies.
;
; THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
; WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
; ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
; WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
; ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
; OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
;
;

        .z80
;
;
;
CPM_VERS  .equ  22              ; version 2.2
BIOS_VERS .equ  10              ; version 1.0
MSIZE     .equ  62              ; memory size in kilobytes

;
;
;
BIAS     .equ   (MSIZE - 20) * 1024
CCP      .equ   0x3400 + BIAS   ; base of cpm console processor
BDOS     .equ   CCP + 0x0806    ; basic dos (resident portion)
BIOS     .equ   CCP + 0x1600    ; base of bios
CDISK    .equ   0x0004          ; address of last logged disk on warm start 0=a,... 15=p
DMA_ADDR .equ   0x0080          ; default buffer address
IOBYTE   .equ   0x0003          ; intel i/o byte

;
; Opcodes for patching
;
JP_OPCODE  .equ 0xC3
JP_Z_OPCDE .equ 0xCA

;
;
CR      .equ 0x0D
LF      .equ 0x0A

;
; eZ80L92 UARTs
;
UART0_LSR .equ 0xC5
UART0_THR .equ 0xC0
UART0_RBR .equ UART0_THR
UART1_LSR .equ 0xD5
UART1_THR .equ 0xD0
UART1_RBR .equ UART1_THR

;
; uint8_t disk_op(uint8_t dsk_op, uint8_t track, uint8_t sector, uint8_t *buffer)
;
disk_op  .equ 0x010008

;
;
;
CB_LEN    .equ 1                ; num coldboot sectors
SECT_LEN  .equ 128
SPT       .equ 26
NUM_FLOPS .equ 4
NUM_SECTS .equ (BIOS - CCP) / 128
DPE_LEN   .equ (dpe1 - dpe0)

;
;
;
        jp      boot
wboote: jp      wboot
        jp      const
        jp      conin
        jp      conout
        jp      list
        jp      punch
        jp      reader
        jp      home
        jp      seldsk
        jp      settrk
        jp      setsec
        jp      setdma
        jp      read
        jp      write
        jp      listst
        jp      sectran
;
; signon message: xxk cp/m vers y.y
;
signon:
        .db     CR, LF, LF
        .db     (MSIZE / 10) +'0 , (MSIZE % 10) + '0
        .fcc    "K CP/M Version "
        .db     (CPM_VERS / 10) + '0, '., (CPM_VERS % 10) + '0
        .fcc    " (eZ80L92 BIOS V"
        .db     (BIOS_VERS / 10) + '0, '., (BIOS_VERS % 10) + '0
        .fcc    " for CP/M 50 Mk II)"
        .db     CR, LF, 0

;
; print signon message and go to ccp
;
boot:
        ld      hl,signon
        call    prmsg
        xor     a
        ld      (IOBYTE),a
        ld      (CDISK),a
        jp      gocpm

;
; On warm boot read boot tracks, skipping cold start loader.
;
wboot:
        ld      sp,DMA_ADDR
        ld      c,0
        call    seldsk
        call    home

        ld      b,NUM_SECTS     ; num sect to load
        ld      c,0             ; start track
        ld      d,CB_LEN + 1    ; start sector
        ld      hl,CCP          ; load_addr

next_sect:
        push    bc
        push    de
        push    hl
        ld      c,d
        call    setsec
        pop     bc
        push    bc
        call    setdma

        call    read
        or      a
        jp      nz,wboot

        pop     hl
        ld      de,SECT_LEN
        add     hl,de
        pop     de
        pop     bc
        dec     b
        jp      z,gocpm

        inc     d
        ld      a,d
        cp      SPT + 1
        jp      c,next_sect

        ld      d,1             ; sectors are 1 based
        inc     c               ; bump track

        call    settrk
        jr      next_sect

;
; enter here from cold start boot
;
gocpm:
        ld      bc,DMA_ADDR
        ld      (dmaad),bc

        ld      a,JP_OPCODE
        ld      (0x0000),a
        ld      hl,wboote       ; wboot entry point
        ld      (0x0001),hl

        ld      (0x0005),a
        ld      hl,BDOS         ; bdos entry point
        ld      (0x0006),hl

        ld      a,(CDISK)       ; last logged disk number
        ld      c,a             ; send to ccp to log it in
        ei
        jp      CCP

;
; console status, return 0xFF if character ready, 0x00 if not available
;
const:
        in0     a,(UART0_LSR)
        and     0x01
        ret     z
        ld      a,0xFF
        ret

;
; console character into register A
;
conin:
        in0     a,(UART0_LSR)
        rra
        jr      nc,conin
        in0     a,(UART0_RBR)
        and     0x7F            ; strip parity bit
        ret

;
; console character output from register C
;
conout:
        in0     a,(UART0_LSR)
        and     0x20
        jr      z,conout
        ld      a,c
        out0    (UART0_THR),a
        ret

;
; list character from register C
;
list:
        ld      a,c
        ret

;
; return list status (0 if not ready, 1 if ready)
;
listst:
        xor     a
        ret

;
; punch character from register C
;
punch:
        in0     a,(UART1_LSR)
        and     0x20
        jr      z,punch
        ld      a,c
        out0    (UART1_THR),a
        ret

;
; read character into register A from reader device
;
reader:
        in0     a,(UART1_LSR)
        rra
        jr      nc,reader
        in0     a,(UART1_RBR)
        and     0x7F
        ret

;
; select disk given by register C
;
seldsk:
        ld      a,c
        cp      NUM_FLOPS       ; flops
        jr      c,selflop
        cp      8               ; hdsk I
        jr      z,selhdsk
        cp      9               ; hdsk J
        jr      z,selhdsk
        ld      hl,0
        ret
selhdsk:
        sub     4               ; normalize hdisk num
        ld      c,a
selflop:
        ld      (diskno),a      ; new disk num

        ld      b,DPE_LEN
        mlt     bc
        ld      hl,dpbase
        add     hl,bc
        ret

;
; move to track 00 position of current drive
;
home:
        ld      bc,0
        ; fall into settrk

;
; set track given by register C
;
settrk:
        ld      (track),bc
        ret

;
; set sector given by register C
;
setsec:
        ld      (sector),bc
        ret

;
; translate the sector given by BC using the translate table given by DE
;
sectran:
        ld      b,0             ; double precision sector number in BC

        ld      a,d
        or      e
        jr      z,notran        ; no translation on null pointer

        ex      de,hl
        add     hl,bc
        ld      l,(hl)
        ld      h,0
        ld      (sector),hl
        ret

notran:
        ld      l,c
        ld      h,b
        inc     hl
        ld      (sector),hl
        ret

;
; set dma address given by registers BC
;
setdma:
        ld      (dmaad),bc
        ret

;
; perform read operation
;
read:
        xor     a               ; set to read function
        jr      rwop            ; perform read function

;
; perform write operation
;
write:
        or      0xFF            ; set to write function
        ; fall into rwop

;
; Enter here from read or write to perform the i/o operation.
; Return 0 in register A if the operation completes properly
; and 1 if an error occurs during the read or write.
;
rwop:
        ld.lil  ix,0
        add.l   ix,sp           ; save SPL
        ld.lil  bc,0            ; clear BC[23:0]
        ld      bc,(dmaad)      ; stack function arguments
        push.l  bc
        ld      bc,(sector)
        push.l  bc
        ld      bc,(track)
        push.l  bc
        ld      bc,(diskno)
        ld      b,a
        push.l  bc
        call.il disk_op
        ld.l    sp,ix           ; restore SPL
        ret

;
; print message at HL until '\0'
;
prmsg:
        ld      a,(hl)
        or      a
        ret     z
        ld      c,a
        call    conout
        inc     hl
        jr      prmsg

;
; data areas
;
track:  .ds     2
sector: .ds     2
dmaad:  .ds     2
diskno: .ds     2

        .include "cpm50mk2dsk.inc"

        .end
