        .setcpu "6502"
        .autoimport on
        .include "pokey_regs.inc"

        .export _ps_serial_install_cio
        .export _ps_serial_remove_cio

        .import _ps_serial_init
        .import _ps_serial_shutdown
        .import _ps_serial_enable_rx_irq
        .import _ps_serial_kick_tx_irq
        .import _ps_serial_rx_rd
        .import _ps_serial_rx_wr
        .import _ps_serial_tx_rd
        .import _ps_serial_tx_wr
        .import _ps_serial_status_accum
        .import _ps_serial_rx_filter_enable
        .import _ps_serial_rx_filter_value
        .import _ps_serial_rx_buf
        .import _ps_serial_tx_buf

HATABS  = $031A
ICCOMZ  = $0022
ICAX1Z  = $002A
ICAX2Z  = $002B
DVSTAT  = $02EA
BRKKEY  = $0236
RTCLOK2 = $0014

.segment "BSS"
_ps_cio_mode_flags:
        .res 1
_ps_cio_rx_enabled:
        .res 1
_ps_cio_iocb_index:
        .res 2
_ps_cio_installed:
        .res 1
_ps_cio_hatabs_offset:
        .res 1

.segment "RODATA"
ps_cio_handler_table:
        .byte <(ps_cio_open-1), >(ps_cio_open-1)
        .byte <(ps_cio_close-1), >(ps_cio_close-1)
        .byte <(ps_cio_get_byte-1), >(ps_cio_get_byte-1)
        .byte <(ps_cio_put_byte-1), >(ps_cio_put_byte-1)
        .byte <(ps_cio_get_status-1), >(ps_cio_get_status-1)
        .byte <(ps_cio_special-1), >(ps_cio_special-1)

.segment "CODE"

_ps_serial_install_cio:
        lda _ps_cio_installed
        beq ps_cio_install_scan
        ldy #$01
        clc
        rts

ps_cio_install_scan:
        ldy #$00
ps_cio_install_loop:
        lda HATABS,y
        beq ps_cio_install_found
        iny
        iny
        iny
        bpl ps_cio_install_loop
        sec
        rts

ps_cio_install_found:
        lda #$4d
        sta HATABS,y
        lda #<ps_cio_handler_table
        iny
        sta HATABS,y
        lda #>ps_cio_handler_table
        iny
        sta HATABS,y
        tya
        sec
        sbc #$02
        sta _ps_cio_hatabs_offset
        lda #$01
        sta _ps_cio_installed
        ldy #$01
        clc
        rts

_ps_serial_remove_cio:
        lda _ps_cio_installed
        beq ps_cio_remove_done
        ldy _ps_cio_hatabs_offset
        lda #$00
        sta HATABS,y
        iny
        sta HATABS,y
        iny
        sta HATABS,y
        lda #$00
        sta _ps_cio_installed
ps_cio_remove_done:
        rts

ps_cio_open:
        cld
        stx _ps_cio_iocb_index
        lda ICAX1Z
        cmp #$0e
        bcc ps_cio_open_ok
        ldy #$a8
        bne ps_cio_return
ps_cio_open_ok:
        sta _ps_cio_mode_flags
        jsr _ps_serial_init
        ldy #$01
ps_cio_return:
        ldx _ps_cio_iocb_index
        rts

ps_cio_close:
        stx _ps_cio_iocb_index
        lda #$16
        jsr ps_cio_delay
        jsr ps_cio_clear_state
        jsr _ps_serial_shutdown
        lda #$3c
        sta PACTL
        ldx _ps_cio_iocb_index
        ldy #$01
        rts

ps_cio_get_byte:
        cld
        stx _ps_cio_iocb_index
        lda _ps_cio_mode_flags
        and #$04
        bne ps_cio_get_ready
        ldy #$83
        bne ps_cio_get_exit
ps_cio_get_ready:
        ldx _ps_cio_rx_enabled
        bne ps_cio_check_break
        ldy #$9a
        bne ps_cio_get_exit
ps_cio_check_break:
        ldy BRKKEY
        bne ps_cio_wait_data
        ldy #$80
        sty BRKKEY
        jmp ps_cio_get_exit
ps_cio_wait_data:
        lda _ps_serial_rx_rd
        eor _ps_serial_rx_wr
        beq ps_cio_check_break
        ldx _ps_serial_rx_rd
        lda _ps_serial_rx_buf,x
        inc _ps_serial_rx_rd
        ldy #$01
ps_cio_get_exit:
        ldx _ps_cio_iocb_index
        rts

ps_cio_put_byte:
        cld
        stx _ps_cio_iocb_index
        tax
        lda _ps_cio_mode_flags
        and #$08
        bne ps_cio_put_ok
        ldy #$87
        jmp ps_cio_put_exit
ps_cio_put_ok:
        txa
        ldx _ps_serial_tx_wr
        sta _ps_serial_tx_buf,x
        inc _ps_serial_tx_wr
        jsr _ps_serial_kick_tx_irq
        ldy #$01
ps_cio_put_exit:
        ldx _ps_cio_iocb_index
        rts

ps_cio_get_status:
        cld
        stx _ps_cio_iocb_index
        ldy #$00
        sty DVSTAT+2
        sty DVSTAT+3
        sty DVSTAT+1
        sty DVSTAT
        lda _ps_serial_status_accum
        sta DVSTAT
        sec
        lda _ps_serial_tx_rd
        sbc _ps_serial_tx_wr
        sta DVSTAT+3
        sec
        lda _ps_serial_rx_wr
        sbc _ps_serial_rx_rd
        sta DVSTAT+1
        ldy #$00
        sta _ps_serial_status_accum
        iny
        ldx _ps_cio_iocb_index
        rts

ps_cio_special:
        cld
        stx _ps_cio_iocb_index
        lda ICAX1Z
        tay
        lda ICCOMZ
        cmp #$28
        beq ps_cio_special_enable
        cmp #$29
        beq ps_cio_special_filter
        ldy #$92
        rts
ps_cio_special_enable:
        lda _ps_cio_mode_flags
        lsr
        bcs ps_cio_special_enable_ok
        ldy #$97
        bne ps_cio_special_done
ps_cio_special_enable_ok:
        lda #$01
        sta _ps_cio_rx_enabled
        jsr _ps_serial_enable_rx_irq
        jmp ps_cio_special_set
ps_cio_special_filter:
        tya
        sta _ps_serial_rx_filter_enable
        lda ICAX2Z
        sta _ps_serial_rx_filter_value
ps_cio_special_set:
        ldy #$01
ps_cio_special_done:
        lda _ps_cio_mode_flags
        sta ICAX1Z
        ldx _ps_cio_iocb_index
        rts

ps_cio_delay:
        clc
        adc RTCLOK2
ps_cio_delay_wait:
        cmp RTCLOK2
        bne ps_cio_delay_wait
        rts

ps_cio_clear_state:
        lda #$00
        sta _ps_serial_rx_rd
        sta _ps_serial_rx_wr
        sta _ps_serial_tx_wr
        sta _ps_serial_tx_rd
        sta _ps_cio_rx_enabled
        sta _ps_cio_mode_flags
        sta _ps_serial_status_accum
        rts
