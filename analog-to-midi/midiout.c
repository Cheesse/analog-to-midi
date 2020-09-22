/*
 * midiout.c
 * MIDI OUTput
 *
 *  Created on: Aug 14, 2020
 *      Author: Andrew Obeso
 */

#include <msp430.h>
#include "midiout.h"

#define MAX_CHARS 16

static volatile int wait = 0;

static char buffer[MAX_CHARS];
static volatile unsigned char index = 0, count = 0;

void midiinit(void) {
    // Reroute pins to LFXIN/LFXOUT functionality
    PJSEL1 &= ~BIT4;
    PJSEL0 |= BIT4;
    // Wait until the oscillator fault flags remain cleared
    CSCTL0 = CSKEY; // Unlock CS registers
    do {
        CSCTL5 &= ~LFXTOFFG; // Local fault flag
        SFRIFG1 &= ~OFIFG; // Global fault flag
    } while (CSCTL5 & LFXTOFFG);
    CSCTL0_H = 0; // Lock CS registers


    /* Initialize eUSCI_A. */
    UCA0CTLW0 |= UCSWRST;

    UCA0CTLW0 |= UCPEN_0 | UCMSB_0 | UC7BIT_0 | UCSPB_0 | UCMODE_0 | UCSYNC_0 | UCSSEL_1;
    UCA0BRW = UCBR1 | UCBR0;
    UCA0MCTLW = UCBRS7 | UCBRS4 | UCBRS1 | UCOS16_0;
    UCA0STATW &= ~UCLISTEN;
    UCA0ABCTL = 0;

    /* Configure Port 2.0 */
    P2SEL1 |= BIT0;

    /* Enable eUSCI_A */
    UCA0CTLW0 &= ~UCSWRST;

    /* Enable interrupts. (Must be done after.) */
    UCA0IE_L = UCTXCPTIE | UCTXIE;
}

#define INDEX_CLAMP(i) ((i) & (unsigned int)(MAX_CHARS - 1))

int midisend(char c) {
    /*if (count == MAX_CHARS) return -1;
    if (UCA0STATW & UCBUSY) {
        buffer[INDEX_CLAMP(index + count)] = c;
        count++;
    } else {*/
        wait = 1;
        UCA0TXBUF = c;
   // }
    return 0;
}

void midiwait(void) {
    while (wait);
}

#pragma vector = EUSCI_A0_VECTOR
interrupt void EUSCIA0InterruptRoutine(void) {
    if (UCA0IFG & UCTXIFG) {
       /* if (count) {
            UCA0TXBUF = buffer[index];
            index = INDEX_CLAMP(index + 1);
            count--;
        } else {
            /* Do nothing. *
        }*/ UCA0IFG &= ~UCTXIFG;
    }
    if (UCA0IFG & UCTXCPTIFG) {
        wait = 0;
        UCA0IFG &= ~UCTXCPTIFG;
    }
}
