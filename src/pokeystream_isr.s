; POKEYStream ISR stubs for OS serial service vectors.
; These are installed into VSERIN/VSEROR and are called by the IRQ handler.

        .export _ps_serial_in_isr
        .export _ps_serial_out_isr

        .import _ps_rx_buf
        .import _ps_rx_w
        .import _ps_tx_isr_flag

SERDAT  = $D20D
XMTDON  = $003A

_ps_serial_in_isr:
        ; Save Y via A (matches MIDI Maze style).
        tya
        pha
        ; Read byte from SERIN and write into RX ring.
        lda SERDAT
        ldy _ps_rx_w
        sta _ps_rx_buf,y
        inc _ps_rx_w
        ; Restore Y and A, then return from IRQ.
        pla
        tay
        pla
        rti

_ps_serial_out_isr:
        ; Output-ready ISR: signal XMTDON for the mainline sender.
        tya
        pha
        lda #$01
        sta XMTDON
        sta _ps_tx_isr_flag
        pla
        tay
        pla
        rti
