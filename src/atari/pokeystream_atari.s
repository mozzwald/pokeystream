        .setcpu "6502"
        .autoimport on
        .include "zeropage.inc"
        .include "pokey_regs.inc"

        .export _ps_init
        .export _ps_shutdown
        .export _ps_send_byte
        .export _ps_send
        .export _ps_recv_byte
        .export _ps_recv
        .export _ps_rx_available
        .export _ps_tx_free

        .import _ps_rx_buf
        .import _ps_tx_buf
        .import _ps_rx_widx
        .import _ps_rx_ridx
        .import _ps_tx_widx
        .import _ps_tx_ridx
        .import _ps_rx_space
        .import _ps_tx_space
        .import _ps_rx_count
        .import _ps_tx_count
        .import _ps_rx_overflow
        .import _ps_last_skstat
        .import _ps_saved_vimirq
        .import _ps_saved_vservecs
        .import _ps_saved_pokmsk
        .import _ps_saved_sskctl
        .import _ps_initialized
        .import _ps_pokey_init

        .import _ps_irq_handler
        .import _ps_vserin
        .import _ps_vseror
        .import _ps_vseroc

.segment "CODE"

; Initialize POKEYStream: vectors, POKEY regs, and IRQ masks.
.proc _ps_init
        php
        sei

        lda _ps_initialized
        beq ps_init_continue
        plp
        rts

ps_init_continue:
        lda POKMSK
        sta _ps_saved_pokmsk
        lda SSKCTL
        sta _ps_saved_sskctl

        lda VIMIRQ
        sta _ps_saved_vimirq
        lda VIMIRQ+1
        sta _ps_saved_vimirq+1

        lda VSERIN
        sta _ps_saved_vservecs
        lda VSERIN+1
        sta _ps_saved_vservecs+1
        lda VSEROR
        sta _ps_saved_vservecs+2
        lda VSEROR+1
        sta _ps_saved_vservecs+3
        lda VSEROC
        sta _ps_saved_vservecs+4
        lda VSEROC+1
        sta _ps_saved_vservecs+5

        lda #<_ps_irq_handler
        sta VIMIRQ
        lda #>_ps_irq_handler
        sta VIMIRQ+1

        lda #<_ps_vserin
        sta VSERIN
        lda #>_ps_vserin
        sta VSERIN+1
        lda #<_ps_vseror
        sta VSEROR
        lda #>_ps_vseror
        sta VSEROR+1
        lda #<_ps_vseroc
        sta VSEROC
        lda #>_ps_vseroc
        sta VSEROC+1

        lda #0
        sta _ps_rx_widx
        sta _ps_rx_ridx
        sta _ps_tx_widx
        sta _ps_tx_ridx

        sta _ps_rx_count
        sta _ps_rx_count+1
        sta _ps_rx_count+2
        sta _ps_rx_count+3
        sta _ps_tx_count
        sta _ps_tx_count+1
        sta _ps_tx_count+2
        sta _ps_tx_count+3
        sta _ps_rx_overflow
        sta _ps_rx_overflow+1
        sta _ps_rx_overflow+2
        sta _ps_rx_overflow+3
        sta _ps_last_skstat

        lda #$00
        sta _ps_rx_space
        sta _ps_tx_space
        lda #$01
        sta _ps_rx_space+1
        sta _ps_tx_space+1

        ; Program POKEY for 19200 baud (SIO-like) as required.
        ldx #8
ps_init_pokey_loop:
        lda _ps_pokey_init,x
        sta AUDF1,x
        dex
        bpl ps_init_pokey_loop

        ; Preserve SSKCTL low bits; route serial per refs/Altirra-850-handler.asm.
        lda SSKCTL
        and #$07
        ora #$70
        sta SSKCTL
        sta SKCTL

        ; Enable serial input IRQ per refs/Altirra-850-handler.asm (ORA #$30).
        ; TX-ready stays off until data queued.
        lda POKMSK
        and #PS_IRQ_SERIAL_CLEAR
        ora #PS_IRQ_RX
        sta POKMSK
        sta IRQEN

        lda #1
        sta _ps_initialized

        plp
        rts
.endproc

; Shutdown: disable serial IRQs and restore vectors/state.
.proc _ps_shutdown
        php
        sei

        lda _ps_initialized
        bne ps_shutdown_continue
        plp
        rts

ps_shutdown_continue:
        ; Clear serial IRQ bits using the Altirra mask ($C7).
        lda POKMSK
        and #PS_IRQ_SERIAL_CLEAR
        sta POKMSK
        sta IRQEN

        lda _ps_saved_vservecs
        sta VSERIN
        lda _ps_saved_vservecs+1
        sta VSERIN+1
        lda _ps_saved_vservecs+2
        sta VSEROR
        lda _ps_saved_vservecs+3
        sta VSEROR+1
        lda _ps_saved_vservecs+4
        sta VSEROC
        lda _ps_saved_vservecs+5
        sta VSEROC+1

        lda _ps_saved_vimirq
        sta VIMIRQ
        lda _ps_saved_vimirq+1
        sta VIMIRQ+1

        lda _ps_saved_pokmsk
        sta POKMSK
        sta IRQEN

        lda _ps_saved_sskctl
        sta SSKCTL
        sta SKCTL

        lda #0
        sta _ps_initialized

        plp
        rts
.endproc

; int ps_send_byte(uint8_t b)
.proc _ps_send_byte
        jsr pusha
        ldx #0
        lda (c_sp,x)

        php
        sei

        lda _ps_tx_space
        ora _ps_tx_space+1
        beq ps_send_byte_full

        ldy _ps_tx_widx
        sta _ps_tx_buf,y
        inc _ps_tx_widx

        sec
        lda _ps_tx_space
        sbc #1
        sta _ps_tx_space
        lda _ps_tx_space+1
        sbc #0
        sta _ps_tx_space+1

        lda POKMSK
        ora #PS_IRQ_TX
        sta POKMSK
        sta IRQEN

        plp
        lda #0
        ldx #0
        jmp incsp1

ps_send_byte_full:
        plp
        lda #$FF
        ldx #$FF
        jmp incsp1
.endproc

; int ps_send(const uint8_t* buf, size_t n)
.proc _ps_send
        jsr pushax

        ldy #$03
        jsr ldptr1ysp
        jsr ldax0sp
        sta tmp1
        stx tmp2

        lda #0
        sta tmp3
        sta tmp4

        lda ptr1
        ora ptr1+1
        bne ps_send_loop_check
        lda tmp1
        ora tmp2
        beq ps_send_done
        lda #$FF
        ldx #$FF
        jmp incsp4

ps_send_loop_check:
        lda tmp1
        ora tmp2
        beq ps_send_done

ps_send_loop:
        lda tmp1
        ora tmp2
        beq ps_send_done

        ldy #0
        lda (ptr1),y

        php
        sei

        lda _ps_tx_space
        ora _ps_tx_space+1
        beq ps_send_full

        ldy _ps_tx_widx
        sta _ps_tx_buf,y
        inc _ps_tx_widx

        sec
        lda _ps_tx_space
        sbc #1
        sta _ps_tx_space
        lda _ps_tx_space+1
        sbc #0
        sta _ps_tx_space+1

        lda POKMSK
        ora #PS_IRQ_TX
        sta POKMSK
        sta IRQEN

        plp

        inc ptr1
        bne ps_send_ptr_ok
        inc ptr1+1
ps_send_ptr_ok:

        sec
        lda tmp1
        sbc #1
        sta tmp1
        lda tmp2
        sbc #0
        sta tmp2

        inc tmp3
        bne ps_send_loop
        inc tmp4
        jmp ps_send_loop

ps_send_full:
        plp

ps_send_done:
        lda tmp3
        ldx tmp4
        jmp incsp4
.endproc

; int ps_recv_byte(uint8_t* out)
.proc _ps_recv_byte
        jsr pushax
        jsr ldptr10sp

        lda ptr1
        ora ptr1+1
        bne ps_recv_byte_check
        lda #0
        ldx #0
        jmp incsp2

ps_recv_byte_check:
        php
        sei

        lda _ps_rx_space
        cmp #$00
        bne ps_recv_byte_has
        lda _ps_rx_space+1
        cmp #$01
        beq ps_recv_byte_empty

ps_recv_byte_has:
        ldy _ps_rx_ridx
        lda _ps_rx_buf,y
        inc _ps_rx_ridx

        ldy #0
        sta (ptr1),y

        clc
        lda _ps_rx_space
        adc #1
        sta _ps_rx_space
        lda _ps_rx_space+1
        adc #0
        sta _ps_rx_space+1

        plp
        lda #1
        ldx #0
        jmp incsp2

ps_recv_byte_empty:
        plp
        lda #0
        ldx #0
        jmp incsp2
.endproc

; size_t ps_recv(uint8_t* buf, size_t maxn)
.proc _ps_recv
        jsr pushax

        ldy #$03
        jsr ldptr1ysp
        jsr ldax0sp
        sta tmp1
        stx tmp2

        lda #0
        sta tmp3
        sta tmp4

        lda ptr1
        ora ptr1+1
        bne ps_recv_loop
        lda tmp1
        ora tmp2
        beq ps_recv_done
        lda #0
        ldx #0
        jmp incsp4

ps_recv_loop:
        lda tmp1
        ora tmp2
        beq ps_recv_done

        php
        sei

        lda _ps_rx_space
        cmp #$00
        bne ps_recv_have
        lda _ps_rx_space+1
        cmp #$01
        beq ps_recv_empty

ps_recv_have:
        ldy _ps_rx_ridx
        lda _ps_rx_buf,y
        inc _ps_rx_ridx
        pha

        clc
        lda _ps_rx_space
        adc #1
        sta _ps_rx_space
        lda _ps_rx_space+1
        adc #0
        sta _ps_rx_space+1

        plp

        ldy #0
        pla
        sta (ptr1),y
        inc ptr1
        bne ps_recv_ptr_ok
        inc ptr1+1
ps_recv_ptr_ok:

        sec
        lda tmp1
        sbc #1
        sta tmp1
        lda tmp2
        sbc #0
        sta tmp2

        inc tmp3
        bne ps_recv_loop
        inc tmp4
        jmp ps_recv_loop

ps_recv_empty:
        plp

ps_recv_done:
        lda tmp3
        ldx tmp4
        jmp incsp4
.endproc

; uint16_t ps_rx_available(void)
.proc _ps_rx_available
        php
        sei

        sec
        lda #$00
        sbc _ps_rx_space
        sta tmp1
        lda #$01
        sbc _ps_rx_space+1
        sta tmp2

        plp
        lda tmp1
        ldx tmp2
        rts
.endproc

; uint16_t ps_tx_free(void)
.proc _ps_tx_free
        php
        sei

        lda _ps_tx_space
        ldx _ps_tx_space+1

        plp
        rts
.endproc
