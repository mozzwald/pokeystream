        .setcpu "6502"
        .autoimport on
        .include "zeropage.inc"
        .include "pokey_regs.inc"

        .export _ps_irq_handler
        .export _ps_vserin
        .export _ps_vseror
        .export _ps_vseroc

        .import _ps_rx_buf
        .import _ps_rx_widx
        .import _ps_rx_ridx
        .import _ps_tx_buf
        .import _ps_tx_widx
        .import _ps_tx_ridx
        .import _ps_rx_space
        .import _ps_tx_space
        .import _ps_rx_count
        .import _ps_tx_count
        .import _ps_rx_overflow
        .import _ps_last_skstat
        .import _ps_saved_vimirq
        .import _ps_isr_byte

.segment "CODE"

; VIMIRQ wrapper: block BREAK per refs/Altirra-850-handler.asm.
_ps_irq_handler:
        bit IRQST
        bpl ps_break_irq
        jmp (_ps_saved_vimirq)

ps_break_irq:
        pha
        lda #$7F
        sta IRQEN
        lda POKMSK
        sta IRQEN
        pla
        rti

; VSERIN handler: RX byte, clear sticky errors via SKREST (Altirra style).
_ps_vserin:
        pha
        txa
        pha
        tya
        pha

        lda SERIN
        sta _ps_isr_byte
        lda SKSTAT
        sta _ps_last_skstat
        lda #$00
        sta SKREST

        ; Count all received bytes (even if dropped).
        inc _ps_rx_count
        bne ps_vserin_count_done
        inc _ps_rx_count+1
        bne ps_vserin_count_done
        inc _ps_rx_count+2
        bne ps_vserin_count_done
        inc _ps_rx_count+3
ps_vserin_count_done:

        lda _ps_rx_space
        ora _ps_rx_space+1
        beq ps_vserin_full

        ldy _ps_rx_widx
        lda _ps_isr_byte
        sta _ps_rx_buf,y
        inc _ps_rx_widx

        sec
        lda _ps_rx_space
        sbc #1
        sta _ps_rx_space
        lda _ps_rx_space+1
        sbc #0
        sta _ps_rx_space+1
        jmp ps_vserin_exit

ps_vserin_full:
        inc _ps_rx_overflow
        bne ps_vserin_exit
        inc _ps_rx_overflow+1
        bne ps_vserin_exit
        inc _ps_rx_overflow+2
        bne ps_vserin_exit
        inc _ps_rx_overflow+3

ps_vserin_exit:
        pla
        tay
        pla
        tax
        pla
        rts

; VSEROR handler: TX ready.
_ps_vseror:
        pha
        txa
        pha
        tya
        pha

        lda _ps_tx_space
        cmp #$00
        bne ps_vseror_has_data
        lda _ps_tx_space+1
        cmp #$01
        beq ps_vseror_empty

ps_vseror_has_data:
        ldy _ps_tx_ridx
        lda _ps_tx_buf,y
        sta SEROUT
        inc _ps_tx_ridx

        clc
        lda _ps_tx_space
        adc #1
        sta _ps_tx_space
        lda _ps_tx_space+1
        adc #0
        sta _ps_tx_space+1

        inc _ps_tx_count
        bne ps_vseror_exit
        inc _ps_tx_count+1
        bne ps_vseror_exit
        inc _ps_tx_count+2
        bne ps_vseror_exit
        inc _ps_tx_count+3
        jmp ps_vseror_exit

ps_vseror_empty:
        ; Clear serial IRQ bits using Altirra's $C7 mask, then keep RX on.
        lda POKMSK
        and #PS_IRQ_SERIAL_CLEAR
        ora #PS_IRQ_RX
        sta POKMSK
        sta IRQEN

ps_vseror_exit:
        pla
        tay
        pla
        tax
        pla
        rts

; VSEROC handler: unused for one-stop-bit mode.
_ps_vseroc:
        rts
