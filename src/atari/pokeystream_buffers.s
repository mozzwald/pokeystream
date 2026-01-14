        .setcpu "6502"
        .autoimport on

        .export _ps_rx_buf
        .export _ps_tx_buf
        .export _ps_rx_widx
        .export _ps_rx_ridx
        .export _ps_tx_widx
        .export _ps_tx_ridx
        .export _ps_rx_space
        .export _ps_tx_space
        .export _ps_rx_count
        .export _ps_tx_count
        .export _ps_rx_overflow
        .export _ps_last_skstat
        .export _ps_tx_idle
        .export _ps_saved_vimirq
        .export _ps_saved_vservecs
        .export _ps_saved_pokmsk
        .export _ps_saved_sskctl
        .export _ps_initialized
        .export _ps_isr_byte
        .export _ps_pokey_init

.segment "BSS"
_ps_rx_buf:
        .res 256
_ps_tx_buf:
        .res 256

_ps_rx_widx:
        .res 1
_ps_rx_ridx:
        .res 1
_ps_tx_widx:
        .res 1
_ps_tx_ridx:
        .res 1

_ps_rx_space:
        .res 2
_ps_tx_space:
        .res 2

_ps_rx_count:
        .res 4
_ps_tx_count:
        .res 4
_ps_rx_overflow:
        .res 4

_ps_last_skstat:
        .res 1

_ps_tx_idle:
        .res 1

_ps_saved_vimirq:
        .res 2
_ps_saved_vservecs:
        .res 6
_ps_saved_pokmsk:
        .res 1
_ps_saved_sskctl:
        .res 1

_ps_initialized:
        .res 1

_ps_isr_byte:
        .res 1

.segment "RODATA"
_ps_pokey_init:
        .byte $28, $A0, $00, $A0, $28, $A0, $00, $A0, $78
