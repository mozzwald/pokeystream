        .setcpu "6502"
        .autoimport on
        .include "pokey_regs.inc"

        .export _ps_serial_init
        .export _ps_serial_shutdown
        .export _ps_serial_enable_rx_irq
        .export _ps_serial_kick_tx_irq
        .export ps_rx_isr
        .export ps_tx_isr

        .export _ps_serial_rx_rd
        .export _ps_serial_rx_wr
        .export _ps_serial_tx_rd
        .export _ps_serial_tx_wr
        .export _ps_serial_status_accum
        .export _ps_serial_rx_filter_enable
        .export _ps_serial_rx_filter_value
        .export _ps_serial_saved_vser
        .export _ps_serial_initialized
        .export _ps_serial_rx_buf
        .export _ps_serial_tx_buf

.segment "ZEROPAGE"
_ps_serial_rx_rd:
        .res 1
_ps_serial_rx_wr:
        .res 1
_ps_serial_tx_rd:
        .res 1
_ps_serial_tx_wr:
        .res 1

.segment "BSS"
_ps_serial_status_accum:
        .res 1
_ps_serial_rx_filter_enable:
        .res 1
_ps_serial_rx_filter_value:
        .res 1
_ps_serial_saved_vser:
        .res 6
_ps_serial_initialized:
        .res 1

_ps_serial_rx_buf:
        .res 256
_ps_serial_tx_buf:
        .res 256

.segment "RODATA"
ps_serial_vector_table:
        .byte <ps_rx_isr, >ps_rx_isr, <ps_tx_isr, >ps_tx_isr, <ps_tx_isr, >ps_tx_isr

.segment "CODE"

_ps_serial_init:
        sei

        lda _ps_serial_initialized
        beq ps_serial_init_continue
        cli
        rts

ps_serial_init_continue:
        ldx #$05
ps_serial_save_vectors:
        lda VSERIN,x
        sta _ps_serial_saved_vser,x
        dex
        bpl ps_serial_save_vectors

        lda #$00
        sta _ps_serial_rx_rd
        sta _ps_serial_rx_wr
        sta _ps_serial_tx_rd
        sta _ps_serial_tx_wr
        sta _ps_serial_status_accum
        sta _ps_serial_rx_filter_enable
        sta _ps_serial_rx_filter_value

        lda #$3c
        sta PACTL
        sta PBCTL
        lda #$a0
        sta AUDC1
        sta AUDC2
        sta AUDC3
        sta AUDC4
        lda #$15
        sta AUDF3
        lda #$00
        sta AUDF4
        lda SSKCTL
        and #$07
        ora #$10
        sta SSKCTL
        sta SKCTL
        sta SKREST
        lda #$39
        sta AUDCTL

        ldx #$05
ps_serial_install_vectors:
        lda ps_serial_vector_table,x
        sta VSERIN,x
        dex
        bpl ps_serial_install_vectors

        lda #$34
        sta PACTL

        jsr _ps_serial_enable_rx_irq

        lda #$01
        sta _ps_serial_initialized
        cli
        rts

_ps_serial_shutdown:
        sei

        lda _ps_serial_initialized
        bne ps_serial_shutdown_continue
        cli
        rts

ps_serial_shutdown_continue:
        lda POKMSK
        and #$c7
        sta POKMSK
        sta IRQEN

        ldx #$05
ps_serial_restore_vectors:
        lda _ps_serial_saved_vser,x
        sta VSERIN,x
        dex
        bpl ps_serial_restore_vectors

        lda #$3c
        sta PACTL
        sta PBCTL

        ldx #$07
        lda #$00
ps_serial_silence_loop:
        sta AUDF1,x
        dex
        bpl ps_serial_silence_loop

        lda #$00
        sta _ps_serial_initialized
        cli
        rts

_ps_serial_enable_rx_irq:
        sei
        lda POKMSK
        ora #$20
        sta POKMSK
        sta IRQEN
        cli
        rts

_ps_serial_kick_tx_irq:
        sei
        lda POKMSK
        ora #$18
        sta POKMSK
        sta IRQEN
        cli
        rts

ps_rx_isr:
        cld
        txa
        pha
        lda SEROUT
        ldx _ps_serial_rx_filter_enable
        beq ps_rx_store_one
        cmp _ps_serial_rx_filter_value
        beq ps_serial_isr_exit
ps_rx_store_one:
        ldx _ps_serial_rx_wr
        sta _ps_serial_rx_buf,x
        inc _ps_serial_rx_wr
ps_serial_isr_exit:
        lda SKCTL
        sta SKREST
        eor #$ff
        and #$c0
        ora _ps_serial_status_accum
        sta _ps_serial_status_accum
        pla
        tax
        pla
        rti

ps_tx_isr:
        cld
        txa
        pha
        ldx _ps_serial_tx_rd
        cpx _ps_serial_tx_wr
        bne ps_tx_send_one

        lda POKMSK
        and #$e7
        sta POKMSK
        sta IRQEN
        jmp ps_serial_tx_exit

ps_tx_send_one:
        lda _ps_serial_tx_buf,x
        sta SEROUT
        inc _ps_serial_tx_rd
ps_serial_wait_irqen_tx_bit:
        lda IRQEN
        and #$08
        beq ps_serial_wait_irqen_tx_bit

ps_serial_tx_exit:
        pla
        tax
        pla
        rti
