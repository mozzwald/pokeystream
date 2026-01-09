#include "pokeystream.h"

#include <stdint.h>

/* Inline SEI/CLI without relying on <6502.h> */
#define SEI() __asm__("sei")
#define CLI() __asm__("cli")

/* --- POKEY/PIA registers --- */
#define PACTL   (*(volatile uint8_t*)0xD302)
#define PBCTL   (*(volatile uint8_t*)0xD303)

#define AUDF3   (*(volatile uint8_t*)0xD204)
#define AUDC3   (*(volatile uint8_t*)0xD205)
#define AUDF4   (*(volatile uint8_t*)0xD206)
#define AUDC4   (*(volatile uint8_t*)0xD207)
#define AUDCTL  (*(volatile uint8_t*)0xD208)

#define SKREST  (*(volatile uint8_t*)0xD20A)
#define SERDAT  (*(volatile uint8_t*)0xD20D) /* read: SERIN, write: SEROUT */
#define IRQEN   (*(volatile uint8_t*)0xD20E)
#define SKCTL   (*(volatile uint8_t*)0xD20F)

/* OS shadows */
#define POKMSK  (*(volatile uint8_t*)0x0010)
#define SSKCTL  (*(volatile uint8_t*)0x0232)
#define XMTDON  (*(volatile uint8_t*)0x003A)

/* OS vectors */
typedef void (*isr_t)(void);
#define VSERIN  (*(isr_t*)0x020A)
#define VSEROR  (*(isr_t*)0x020C)

/* ISR stubs */
void ps_serial_in_isr(void);
void ps_serial_out_isr(void);

/* Ring buffers */
uint8_t ps_rx_buf[256];
uint8_t ps_tx_buf[256];

volatile uint8_t ps_rx_w = 0;
volatile uint8_t ps_rx_r = 0;
volatile uint8_t ps_tx_w = 0;
volatile uint8_t ps_tx_r = 0;
volatile uint8_t ps_tx_isr_flag = 0;

static isr_t old_vserin = 0;
static isr_t old_vseror = 0;
static uint8_t old_pokmsk = 0;
static uint8_t initialized = 0;

static void apply_midi_maze_settings(void)
{
    uint8_t tmp = SSKCTL;

    AUDCTL = 0x39;
    AUDC3  = 0xA0;
    AUDC4  = 0xA0;
    AUDF3  = 0x15;
    AUDF4  = 0x00;

    tmp = (uint8_t)((tmp & 0x07) | 0x10);
    SSKCTL = tmp;
    SKCTL  = tmp;
    SKREST = tmp;
}

static void apply_tx_settings(void)
{
    /* SIO-style setup used by the working sample for TX pacing. */
    SSKCTL = 0x73;
    SKCTL  = 0x73;

    AUDC4  = 0xA0;
    AUDF3  = 0x08;
    AUDF4  = 0x00;
    AUDCTL = 0x28;
    SKCTL  = 0x23;
    SKREST = 0x01;
}

int ps_init(void)
{
    SEI();

    if (initialized) {
        CLI();
        return 0;
    }

    PACTL = 0x3C;
    PBCTL = 0x3C;

    old_vserin = VSERIN;
    old_vseror = VSEROR;
    old_pokmsk = POKMSK;

    VSERIN = ps_serial_in_isr;
    VSEROR = ps_serial_out_isr;

    ps_rx_w = ps_rx_r = 0;
    ps_tx_w = ps_tx_r = 0;
    ps_tx_isr_flag = 0;

    apply_midi_maze_settings();

    POKMSK = (uint8_t)((POKMSK & 0xF0) | 0x30);
    IRQEN  = POKMSK;

    CLI();
    PACTL = 0x34;

    initialized = 1;
    return 0;
}

void ps_shutdown(void)
{
    SEI();

    if (!initialized) {
        CLI();
        return;
    }

    POKMSK = old_pokmsk;
    IRQEN  = old_pokmsk;
    VSERIN = old_vserin;
    VSEROR = old_vseror;

    initialized = 0;
    CLI();
}

bool ps_try_send_byte(uint8_t b)
{
    if ((uint8_t)(ps_tx_w - ps_tx_r) == 0xFF) {
        return false;
    }
    ps_tx_buf[ps_tx_w++] = b;
    return true;
}

void ps_send_byte(uint8_t b)
{
    while (!ps_try_send_byte(b)) {
        /* spin */
    }
}

uint16_t ps_send(const void* data, uint16_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint16_t i = 0;
    while (i < len && ps_try_send_byte(p[i])) {
        ++i;
    }
    return i;
}

uint8_t ps_rx_available(void)
{
    return (uint8_t)(ps_rx_w - ps_rx_r);
}

bool ps_read_byte(uint8_t* out)
{
    if (ps_rx_r == ps_rx_w) return false;
    *out = ps_rx_buf[ps_rx_r++];
    return true;
}

uint16_t ps_read(void* dst, uint16_t maxlen)
{
    uint8_t* p = (uint8_t*)dst;
    uint16_t n = 0;
    while (n < maxlen && ps_read_byte(&p[n])) {
        ++n;
    }
    return n;
}

void ps_flush_tx(void)
{
    apply_tx_settings();

    while (ps_tx_r != ps_tx_w) {
        XMTDON = 0;
        SERDAT = ps_tx_buf[ps_tx_r++];
        while (XMTDON == 0) {
            /* spin */
        }
    }

    apply_midi_maze_settings();
}
