/*
 * atmcmain.c
 *
 * Analog-to-MIDI Converter MAIN
 *  Created on: Sep 18, 2020
 *      Author: andob
 */

#include <msp430.h>
#include "fsg.h"

void main(void) {
    /* Do stuff. */
    P1DIR = 0xFF;
    P2DIR = 0xFF;
    P3DIR = 0xFF;
    P4DIR = 0xFF;
    P5DIR = 0xFF;
    P6DIR = 0xFF;
    P7DIR = 0xFF;
    P8DIR = 0xFF;
    PADIR = 0xFF;
    PBDIR = 0xFF;
    PCDIR = 0xFF;
    PDDIR = 0xFF;

    PM5CTL0 &= ~LOCKLPM5;

    for (;;) {
        fsgupdate(0);
        P1OUT ^= BIT0;
    }
}
