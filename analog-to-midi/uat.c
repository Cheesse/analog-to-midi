/*
 * uat.c
 *
 * UAT Driver
 *  Created on: Nov 18, 2020
 *      Author: andob
 */

#include "config.h"
#include "defs.h"

#include "cpu.h"
#include "uat.h"

#include <msp430.h>

#define MAX_CHARS 16

static volatile int wait = 0;

static char buffer[MAX_CHARS];
static volatile unsigned char index = 0, count = 0;

#define clamp(i) ((i) & (unsigned int)(MAX_CHARS - 1))

/* Sets the baud rate based on the configuration. Assumes the source clock is ACLK @ 500 kHz. */
static inline void setbaud(void) {
#if UAT_BAUD_RATE == 31250
    /* UCA1BRW: eUSCI A1 Baud Rate Control Register.
     *  UCBR (Clock Prescaler) = SMCLK_FREQ / UAT_BAUD_RATE / 16 */
    UCA1BRW = SMCLK_FREQ / UAT_BAUD_RATE / 16;

    /* UCA1MCTLW: eUSCI A1 Modulation Control Register.
     *  UCBRS  (2nd Modulation Stage) = UAT_BAUD_MOD
     *  UCBRF  (1st Modulation Stage) = SMCLK_FREQ / UAT_BAUD_RATE % 16
     *  UCOS16 (Oversampling Mode)    = 1 (Enabled) */
    UCA1MCTLW = (UAT_BAUD_MOD << 8) | ((SMCLK_FREQ / UAT_BAUD_RATE % 16) << 4) | UCOS16_1;
#else
#error "UAT baud rate not set to a predefined value. "
#endif /* UAT_BAUD_RATE */
}

void uatenable(void) {
    /* Enable eUSCI A1 */
    UCA1CTLW0 &= ~UCSWRST;

    /* Enable interrupts. (Must be done after.) */
    UCA1IE_L = UCTXIE;
}

void uatinit(void) {
    /* UCA1CTLW0: eUSCI A1 Control Register 0.
     *  UCPEN    (Parity Enable)     = 0 (Disabled)
     *  UCPAR    (Parity Select)     = X (Don't care)
     *  UCMSB    (MSB First Select)  = 0 (LSB first)
     *  UC7BIT   (Character Length)  = 0 (8 bits)
     *  UCSPB    (Stop Bit Count)    = 0 (1 stop bit)
     *  UCMODE   (Asynchronous Mode) = 0 (UART mode)
     *  UCSYNC   (Synchronous Mode)  = 0 (Disable)
     *  UCSSEL   (Clock Source)      = 2 (SMCLK)
     *  UCBRXEIE (Bad Character IE)  = X (Don't care)
     *  UCBRKIE  (Break Received IE) = X (Don't care)
     *  UCDORM   (Sleep Mode)        = 0 (Disable)
     *  UCTXADDR (Transmit Address)  = 0 (No)
     *  UCTXBRK  (Transmit Break)    = 0 (No) */
    UCA1CTLW0 = UCPEN_0 | UCMSB_0 | UC7BIT_0 | UCSPB_0 | UCMODE_0 | UCSYNC_0 | UCSSEL_2 | UCSWRST;

    setbaud();

    /* UCA1STATW: eUSCI A1 Status Register.
     * UCLISTEN (Loopback Mode) = 0 (Disable) */
    UCA1STATW = 0;

    /* Set this to 0. */
    UCA1ABCTL = 0;
}

int uatoutc(char c) {
    unsigned int a;

    /* Disable interrupts for this module to avoid race conditions. */
    UCA1IE_L = 0;

    if (count == MAX_CHARS) {
        /* Re-enable interrupts. */
        UCA1IE_L = UCTXIE;

        return 1;
    }

    buffer[clamp(index + count)] = c;
    a = count;
    count++;
    if (!a) {
        wait = 1;
        UCA1TXBUF = c;
    }

    /* Re-enable interrupts. */
    UCA1IE_L = UCTXIE;

    return 0;
}

int uatouts(const char *s, unsigned int n) {
    unsigned int a, b, i;

    /* Disable interrupts for this module to avoid race conditions. */
    UCA1IE_L = 0;

    a = count + n;
    if (a >= MAX_CHARS) {
        /* Re-enable interrupts. */
        UCA1IE_L = UCTXIE;

        return 1;
    }

    b = index + count;
    for (i = 0; i != n; i++) buffer[clamp(b + i)] = s[i];

    b = count;
    count = a;
    if (!b) {
        wait = 1;
        UCA1TXBUF = buffer[index];
    }

    /* Re-enable interrupts. */
    UCA1IE_L = UCTXIE;

    return 0;
}

void uatwait(void) { while (wait) __low_power_mode_0(); }

#pragma vector = EUSCI_A1_VECTOR
RAMFUNC interrupt void EUSCIA1InterruptRoutine(void) {
    if (count) {
        count--;
        index = clamp(index + 1);

        if (count) UCA1TXBUF = buffer[index];
        else {
            wait = 0;
            UCA1IFG &= ~UCTXIFG;
            __low_power_mode_off_on_exit();
        }
    } else UCA1IFG &= ~UCTXIFG;
}
