;
; Copyright (c) 2018 Jesse Marroquin
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
;
;
        include "ez80l92.inc"

        xref __CS0_CTL_INIT_PARAM
        xref __CS0_BMC_INIT_PARAM
        xref __stack
        xref __low_bss
        xref __len_bss
        xref __low_data
        xref __len_data
        xref __low_romdata
        xref _main
        xref _dskop
        xref _dskman

        xdef disk_op_madl
        xdef disk_man_madl
;
;
;
        define .RESET, space = ROM
        segment .RESET
        .assume ADL = 1

 _reset:
        di
        rsmix                       ; clear MADL
        jp.lil _l92init             ; transition to ADL mode
;
; CP/M entry points at a fixed point in memory
;
        .assume ADL = 1

disk_op_madl:
        call _dskop
        ret.l

disk_man_madl:
        call _dskman
        ret.l


        define .STARTUP, space = ROM
        segment .STARTUP
        .assume ADL = 1

 _l92init:
        ld      a,__CS0_BMC_INIT_PARAM
        out0    (CS0_BMC),a
        ld      a,__CS0_CTL_INIT_PARAM
        out0    (CS0_CTL),a

        ld      sp,__stack

        ; clear BSS
        ld      bc,__len_bss - 1    ; copy cnt
        ld      hl,__low_bss        ; src
        ld      de,hl
        inc     de                  ; dest
        xor     a
        ld      (hl),a              ; clr repl tgt
        ldir

        ; copy DATA
        ld      bc,__len_data       ; cnt
        ld      a, __len_data >> 16
        or      a,c
        or      a,b
        jr      z,_data_done
        ld      hl,__low_romdata    ; src
        ld      de,__low_data       ; dest
        ldir
_data_done:

        call    _main               ; void main(void)
;
; C library exit points
;
__exit:
_exit:
_abort:
        jp      $

        end
