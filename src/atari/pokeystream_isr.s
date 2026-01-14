        .setcpu "6502"
        .autoimport on
        .include "zeropage.inc"
        .include "pokey_regs.inc"

        .export _ps_irq_handler
        .export _ps_vserin
        .export _ps_vseror
        .export _ps_vseroc
        .export _ps_swap_irq_vector

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
        .import _ps_isr_byte
        .import _ps_tx_idle

.segment "CODE"

; VIMIRQ wrapper: block BREAK per refs/Altirra-850-handler.asm.
_ps_irq_handler:
        bit IRQST
        bpl ps_break_irq         ; BREAK asserted (bit7=0) -> handle
ps_chain:
        jmp $FFFF                ; patched to old VIMIRQ
_ps_irq_chain_addr = *-2

ps_break_irq:
        pha
        lda #$7F
        sta IRQEN
        lda POKMSK
        sta IRQEN
        pla
        jmp ps_chain

; Swap VIMIRQ vector with our chain address, matching Altirra's SwapIrqVector.
_ps_swap_irq_vector:
        ldx #1
ps_swap_loop:
        lda VIMIRQ,x
        pha
        lda _ps_irq_chain_addr,x
        sta VIMIRQ,x
        pla
        sta _ps_irq_chain_addr,x
        dex
        bpl ps_swap_loop
        rts

; VSERIN handler: RX byte, SKSTAT capture + SKREST clear (Altirra style).
_ps_vserin:
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
        rti

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
        lda #$00
        sta _ps_tx_idle
        ldx _ps_tx_ridx
        lda _ps_tx_buf,x
        sta SEROUT
        inx
        stx _ps_tx_ridx

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
        lda #$01
        sta _ps_tx_idle
        lda POKMSK
        and #PS_IRQ_TX_CLEAR
        sta POKMSK
        sta IRQEN
        jmp ps_vseror_exit

ps_vseror_exit:
        pla
        tay
        pla
        tax
        pla
        rti

; VSEROC handler: output complete; clear the IRQ per Altirra handler.
_ps_vseroc:
        pha
        lda POKMSK
        and #$F7
        sta POKMSK
        sta IRQEN
        pla
        rti
