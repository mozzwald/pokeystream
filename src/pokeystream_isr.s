; POKEYStream IRQ wrapper + serial ISR routines (850-style).
; Installs a VIMIRQ wrapper that services serial IRQs, then chains to OS.

        .export _ps_irq_wrapper
        .export _ps_isr_rx
        .export _ps_isr_tx

        .import _ps_old_vimirq
        .import _ps_rx_buf
        .import _ps_rx_w
        .import _ps_rx_r
        .import _ps_tx_buf
        .import _ps_tx_w
        .import _ps_tx_r
        .import _ps_tx_active
        .import _ps_rx_bytes
        .import _ps_tx_bytes
        .import _ps_rx_overflow
        .import _ps_last_skstat_error

IRQST   = $D20E
IRQEN   = $D20E
SERDAT  = $D20D
SKSTAT  = $D20F
SKREST  = $D20A
POKMSK  = $0010

IRQ_RX_MASK = $20
IRQ_TX_MASK = $10
IRQ_SERIAL_MASK = IRQ_RX_MASK | IRQ_TX_MASK

        .segment "BSS"
ps_irqst_snapshot: .res 1

        .segment "CODE"

_ps_irq_wrapper:
        pha
        txa
        pha
        tya
        pha

        lda IRQST
        sta ps_irqst_snapshot
        and #IRQ_SERIAL_MASK
        cmp #IRQ_SERIAL_MASK
        beq @chain

        lda ps_irqst_snapshot
        and #IRQ_RX_MASK
        bne @no_rx
        jsr _ps_isr_rx
@no_rx:
        lda ps_irqst_snapshot
        and #IRQ_TX_MASK
        bne @no_tx
        jsr _ps_isr_tx
@no_tx:
@chain:
        pla
        tay
        pla
        tax
        pla
        jmp (_ps_old_vimirq)

_ps_isr_rx:
        lda SERDAT
        pha
        lda SKSTAT
        sta _ps_last_skstat_error
        sta SKREST
        pla

        ldx _ps_rx_w
        inx
        cpx _ps_rx_r
        bne @store
        inc _ps_rx_overflow
        bne @overflow_ok
        inc _ps_rx_overflow+1
@overflow_ok:
        inc _ps_rx_r
@store:
        ldx _ps_rx_w
        sta _ps_rx_buf,x
        inx
        stx _ps_rx_w

        inc _ps_rx_bytes
        bne @rx_bytes_ok
        inc _ps_rx_bytes+1
@rx_bytes_ok:
        rts

_ps_isr_tx:
        ldx _ps_tx_r
        cpx _ps_tx_w
        beq @empty
        lda _ps_tx_buf,x
        sta SERDAT
        inx
        stx _ps_tx_r

        inc _ps_tx_bytes
        bne @done
        inc _ps_tx_bytes+1
        bne @done
@empty:
        lda #$00
        sta _ps_tx_active
        lda POKMSK
        and #$EF
        sta POKMSK
        sta IRQEN
@done:
        rts
