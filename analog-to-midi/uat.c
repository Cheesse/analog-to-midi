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

#define MAX_CHARS 64

static volatile int wait = 0;

static char buffer[MAX_CHARS];
static volatile unsigned char index = 0, count = 0;

#define clamp(i) ((i) & (unsigned int)(MAX_CHARS - 1))

/* Sets the baud rate based on the configuration. Assumes the source clock is ACLK @ 500 kHz. */
static inline void setbaud(void) {
    /* UCA1BRW: eUSCI A1 Baud Rate Control Register.
     *  UCBR (Clock Prescaler) = SMCLK_FREQ / UAT_BAUD_RATE / 16 */
    UCA1BRW = SMCLK_FREQ / UAT_BAUD_RATE / 16;

    /* UCA1MCTLW: eUSCI A1 Modulation Control Register.
     *  UCBRS  (2nd Modulation Stage) = UAT_BAUD_MOD
     *  UCBRF  (1st Modulation Stage) = SMCLK_FREQ / UAT_BAUD_RATE % 16
     *  UCOS16 (Oversampling Mode)    = 1 (Enabled) */
    UCA1MCTLW = (UAT_BAUD_MOD << 8) | ((SMCLK_FREQ / UAT_BAUD_RATE % 16) << 4) | UCOS16_1;
}

void uatenable(void) {
    /* Enable eUSCI A1 */
    UCA1CTLW0 &= ~UCSWRST;

    /* Enable interrupts. (Must be done after.) */
    UCA1IE_L = UCTXCPTIE;
}

void uatinit(void) {
    /* First set up the sample-and-hold timer. We can use the TA0 CCR1 timer. */

    /* TA0CCR0: Timer A Capture/Compare Register 0. */
    //TA1CCR0 = SMCLK_FREQ / UAT_BAUD_RATE / 16;

    /* TA0CCTL1: Timer A0 Capture/Compare Control Register
     *  CM     (Capture Mode) = 0 (None)
     *  CCIS   (Input Select)         = 0 (CCI1A/TA0.1)
     *  SCS    (Capture Source Sync)  = X (Don't care)
     *  SCCI   (C/C Input Sync)       = X (Don't care)
     *  CAP    (Capture/Compare Mode) = 0 (Compare)
     *  OUTMOD (Output Mode)          = 4 (Toggle)
     *  CCIE   (C/C Interrupt Enable) = 0 (No)
     *  CCI    (C/C Input)            = X (Don't care)
     *  OUT    (Output Mode)          = X (Don't care)
     *  COV    (Capture Overflow)     = X (Don't care)
     *  CCIFG (C/C Interrupt Flag)    = X (Don't care) */
    //TA0CCTL1 = CM_0 | CCIS_0 | CAP_0 | OUTMOD_4 | CCIE_0;

    /* TA0CTL: Timer A0 Control Register.
     *  TASSEL (Clock Source Select) = 2 (SMCLK)
     *  ID     (Input Clock Divider) = 0 (1)
     *  MC     (Mode Control)        = 0 (Stop mode)
     *  TACLR  (Clear Timer)         = X (Don't care)
     *  TAIE   (Interrupt Enable)    = 0 (No)
     *  TAIFG  (Interrupt Flag)      = X (Don't care) */
    //TA0CTL = TASSEL_2 | ID_0 | MC_0 | TAIE_0;

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
        UCA1IE_L = UCTXCPTIE;

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
    UCA1IE_L = UCTXCPTIE;

    return 0;
}

int uatouts(const char *s, unsigned int n) {
    unsigned int a, b, i;

    /* Disable interrupts for this module to avoid race conditions. */
    UCA1IE_L = 0;

    a = count + n;
    if (a >= MAX_CHARS) {
        /* Re-enable interrupts. */
        UCA1IE_L = UCTXCPTIE;

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
    UCA1IE_L = UCTXCPTIE;

    return 0;
}

void uatwait(void) { while (wait) __low_power_mode_0(); }

#pragma vector = EUSCI_A1_VECTOR
RAMFUNC interrupt void EUSCIA1InterruptRoutine(void) {
    if (count) {
        count--;
        index = clamp(index + 1);

        if (count) {
            UCA1TXBUF = buffer[index];
            UCA1IFG &= ~UCTXCPTIFG;
        }
        else {
            wait = 0;
            UCA1IFG &= ~UCTXCPTIFG;
            __low_power_mode_off_on_exit();
        }
    } else UCA1IFG &= ~UCTXCPTIFG;
}
