/*
 * duart.c
 *
 * Debug UART Driver
 *  Created on: Nov 18, 2020
 *      Author: andob
 */

#include "config.h"
#include "defs.h"

#include "cpu.h"
#include "duart.h"

#include <msp430.h>

/* This UART will be used to send up to around 320 bytes at a time. */
#define MAX_CHARS 512

static volatile int wait = 0;

static char buffer[MAX_CHARS];
static volatile unsigned int index = 0, count = 0;

#define clamp(i) ((i) & (MAX_CHARS - 1))

/* Sets the baud rate based on the configuration. Assumes the source clock is SMCLK @ 1.25 MHz. */
static inline void setbaud(void) {
#if DEBUG_UART_BAUD_RATE == 9600
    /* UCA0BRW: eUSCI A0 Baud Rate Control Register.
     *  UCBR (Clock Prescaler) = SMCLK_FREQ / DEBUG_UART_BAUD_RATE / 16 */
    UCA0BRW = SMCLK_FREQ / DEBUG_UART_BAUD_RATE / 16;

    /* UCA0MCTLW: eUSCI_A0 Modulation Control Register.
     *  UCBRS  (2nd Modulation Stage) = 0x00
     *  UCBRF  (1st Modulation Stage) = SMCLK_FREQ / DEBUG_UART_BAUD_RATE % 16
     *  UCOS16 (Oversampling Mode)    = 1 (Enabled) */
    UCA0MCTLW = (0x00 << 8) | ((SMCLK_FREQ / DEBUG_UART_BAUD_RATE % 16) << 4) | UCOS16_1;
#elif DEBUG_UART_BAUD_RATE == 115200
    /* UCA0BRW: eUSCI A0 Baud Rate Control Register.
     *  UCBR (Clock Prescaler) = SMCLK_FREQ / DEBUG_UART_BAUD_RATE */
    UCA0BRW = SMCLK_FREQ / DEBUG_UART_BAUD_RATE;

    /* UCA0MCTLW: eUSCI_A0 Modulation Control Register.
     *  UCBRS  (2nd Modulation Stage) = 0x92
     *  UCBRF  (1st Modulation Stage) = X (Don't care)
     *  UCOS16 (Oversampling Mode)    = 0 (Disabled) */
    UCA0MCTLW = (0x92 << 8) | UCOS16_0;
#else
#error "Debug UART baud rate not set to a predefined value. "
#endif /* DEBUG_UART_BAUD_RATE */
}

void duartenable(void) {
    /* Enable eUSCI A0 */
    UCA0CTLW0 &= ~UCSWRST;

    /* Enable interrupts. (Must be done after.) */
    UCA0IE_L = UCTXIE;
}

void duartinit(void) {
    /* UCA0CTLW0: eUSCI A0 Control Register 0.
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
    UCA0CTLW0 = UCPEN_0 | UCMSB_0 | UC7BIT_0 | UCSPB_0 | UCMODE_0 | UCSYNC_0 | UCSSEL_2 | UCSWRST;

    setbaud();

    /* UCA0STATW: eUSCI A0 Status Register.
     * UCLISTEN (Loopback Mode) = 0 (Disable) */
    UCA0STATW = 0;

    /* Set this to 0. */
    UCA0ABCTL = 0;
}

int duartoutc(char c) {
    unsigned int a;

    /* Disable interrupts for this module to avoid race conditions. */
    UCA0IE_L = 0;

    if (count == MAX_CHARS) {
        /* Re-enable interrupts. */
        UCA0IE_L = UCTXIE;

        return 1;
    }

    buffer[clamp(index + count)] = c;
    a = count;
    count++;
    if (!a) {
        wait = 1;
        UCA0TXBUF = c;
    }

    /* Re-enable interrupts. */
    UCA0IE_L = UCTXIE;

    return 0;
}

int duartouts(const char *s, unsigned int n) {
    unsigned int a, b, i;

    /* Disable interrupts for this module to avoid race conditions. */
    UCA0IE_L = 0;

    a = count + n;
    if (a >= MAX_CHARS) {
        /* Re-enable interrupts. */
        UCA0IE_L = UCTXIE;

        return 1;
    }

    b = index + count;
    for (i = 0; i != n; i++) buffer[clamp(b + i)] = s[i];

    b = count;
    count = a;
    if (!b) {
        wait = 1;
        UCA0TXBUF = buffer[index];
    }

    /* Re-enable interrupts. */
    UCA0IE_L = UCTXIE;

    return 0;
}

void duartwait(void) { while (wait) __low_power_mode_0(); }

#pragma vector = EUSCI_A0_VECTOR
RAMFUNC interrupt void EUSCIA0InterruptRoutine(void) {
    if (count) {
        count--;
        index = clamp(index + 1);

        if (count) UCA0TXBUF = buffer[index];
        else {
            wait = 0;
            UCA0IFG &= ~UCTXIFG;
            __low_power_mode_off_on_exit();
        }
    } else UCA0IFG &= ~UCTXIFG;
}
