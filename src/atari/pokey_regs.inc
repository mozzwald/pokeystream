; POKEYStream register and vector equates.
; Values follow Atari OS and POKEY documentation.

AUDF1   = $D200
AUDC1   = $D201
AUDF2   = $D202
AUDC2   = $D203
AUDF3   = $D204
AUDC3   = $D205
AUDF4   = $D206
AUDC4   = $D207
AUDCTL  = $D208

SKREST  = $D20A

SERIN   = $D20D
SEROUT  = $D20D

IRQST   = $D20E
IRQEN   = $D20E

SKSTAT  = $D20F
SKCTL   = $D20F

PACTL   = $D302
PBCTL   = $D303

POKMSK  = $0010
SSKCTL  = $0232

VIMIRQ  = $0216
VSERIN  = $020A
VSEROR  = $020C
VSEROC  = $020E

; IRQ masks per refs/Altirra-850-handler.asm (serial bits are $30).
PS_IRQ_SERIAL_MASK  = $30
PS_IRQ_SERIAL_CLEAR = $C7
PS_IRQ_RX           = $10
PS_IRQ_TX           = $20
PS_IRQ_TX_CLEAR     = $DF
