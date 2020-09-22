/*
 * atmcmain.c
 *
 * Analog-to-MIDI Converter MAIN
 *  Created on: Sep 18, 2020
 *      Author: andob
 */

#include <msp430.h>
#include "adcd.h"
#include "fsg.h"
#include "midiout.h"

void init(void) {
    /* Set FRAM to have 1 wait state in preparation for the increased clock speed. */
    FRCTL0_L |= NWAITS1;

    /* Increase MCU clock rate from 8 MHz to 16 MHz. */
    CSCTL0 = CSKEY; // Unlock CS registers
    CSCTL1 = DCORSEL | DCOFSEL_4;
    CSCTL3 &= ~DIVM;
    CSCTL0_H = 0; // Lock CS registers

    WDTCTL = WDTPW | WDTHOLD; // Stop WDT

    P1DIR = 0xFF;
    P2DIR = 0xFF;
    P3DIR = 0xFF;
    P4DIR = 0xFF;
    P5DIR = 0xFF & ~BIT6;
    P6DIR = 0xFF;
    P7DIR = 0xFF;
    P8DIR = 0xFF;
    PADIR = 0xFF;
    PBDIR = 0xFF;
    PCDIR = 0xFF;
    PDDIR = 0xFF;

    PM5CTL0 &= ~LOCKLPM5;
    _enable_interrupts();
}

void main(void) {
    init();
    adcinit();
    fsginit();
    midiinit();

    P5OUT |= BIT6;

    for (;;) {
        int sample = adcsample();
        fsgupdate(sample);
    }
}
