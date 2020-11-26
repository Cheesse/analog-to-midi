/*
 * io.c
 *
 * Input/Output Port Driver
 *  Created on: Nov 17, 2020
 *      Author: andob
 */

#include "config.h"
#include "defs.h"

#include "io.h"

#include <msp430.h>

/* Interrupt handlers for each port. */
static iofunc porthandler[IOPORT_COUNT];

static char portimask[IOPORT_COUNT];

void ioenable(void) {
    /* Unlock I/O ports. */
    PM5CTL0 &= ~LOCKLPM5;

    __enable_interrupt();
}

char ioget(ioport port, char mask) {
    switch (port) {
    case IOPORT_1: return P1IN & mask;
    case IOPORT_2: return P2IN & mask;
    case IOPORT_3: return P3IN & mask;
    case IOPORT_4: return P4IN & mask;
    case IOPORT_5: return P5IN & mask;
    case IOPORT_6: return P6IN & mask;
    case IOPORT_7: return P7IN & mask;
    case IOPORT_8: return P8IN & mask;
    case IOPORT_J: return PJIN & mask;
    } return 0;
}

void iohandler(ioport port, char mask0, char mask1, iofunc handler) {
    porthandler[port] = handler;
    portimask[port] = mask0;

    switch (port) {
    case IOPORT_1:
        P1IE |= mask0;
        P1IES ^= mask1;
        return;
    case IOPORT_2:
        P2IE |= mask0;
        P2IES ^= mask1;
        return;
    case IOPORT_3:
        P3IE |= mask0;
        P3IES ^= mask1;
        return;
    case IOPORT_4:
        P4IE |= mask0;
        P4IES ^= mask1;
        return;
    case IOPORT_5:
        P5IE |= mask0;
        P5IES ^= mask1;
        return;
    case IOPORT_6:
        P6IE |= mask0;
        P6IES ^= mask1;
        return;
    case IOPORT_7:
        P7IE |= mask0;
        P7IES ^= mask1;
        return;
    case IOPORT_8:
        P8IE |= mask0;
        P8IES ^= mask1;
        return;
    /*case IOPORT_J:
        PJIE |= mask0;
        PJIES ^= mask1;
        return;*/
    }
}

void ioin(ioport port, char mask0, char mask1) {
    switch (port) {
    case IOPORT_1:
        P1DIR &= ~mask0;
        P1REN |= mask1;
        return;
    case IOPORT_2:
        P2DIR &= ~mask0;
        P2REN |= mask1;
        return;
    case IOPORT_3:
        P3DIR &= ~mask0;
        P3REN |= mask1;
        return;
    case IOPORT_4:
        P4DIR &= ~mask0;
        P4REN |= mask1;
        return;
    case IOPORT_5:
        P5DIR &= ~mask0;
        P5REN |= mask1;
        return;
    case IOPORT_6:
        P6DIR &= ~mask0;
        P6REN |= mask1;
        return;
    case IOPORT_7:
        P7DIR &= ~mask0;
        P7REN |= mask1;
        return;
    case IOPORT_8:
        P8DIR &= ~mask0;
        P8REN |= mask1;
        return;
    case IOPORT_J:
        PJDIR &= ~mask0;
        PJREN |= mask1;
        return;
    }
}

void ioinit(void) {
    /* Set unused I/O ports to output. */
    PADIR = 0xFFFF;
    PAOUT = 0x0000;
    PAIES = 0x0000;
    PAIFG = 0x0000;
    PBDIR = 0xFFFF;
    PBOUT = 0x0000;
    PBIES = 0x0000;
    PBIFG = 0x0000;
    PCDIR = 0xFFFF;
    PCOUT = 0x0000;
    PCIES = 0x0000;
    PCIFG = 0x0000;
    PDDIR = 0xFFFF;
    PDOUT = 0x0000;
    PDIES = 0x0000;
    PDIFG = 0x0000;
}

void iomode(ioport port, char mask0, char mask1) {
    switch (port) {
    case IOPORT_1:
        P1SEL0 |= mask0;
        P1SEL1 |= mask1;
        return;
    case IOPORT_2:
        P2SEL0 |= mask0;
        P2SEL1 |= mask1;
        return;
    case IOPORT_3:
        P3SEL0 |= mask0;
        P3SEL1 |= mask1;
        return;
    case IOPORT_4:
        P4SEL0 |= mask0;
        P4SEL1 |= mask1;
        return;
    case IOPORT_5:
        P5SEL0 |= mask0;
        P5SEL1 |= mask1;
        return;
    case IOPORT_6:
        P6SEL0 |= mask0;
        P6SEL1 |= mask1;
        return;
    case IOPORT_7:
        P7SEL0 |= mask0;
        P7SEL1 |= mask1;
        return;
    case IOPORT_8:
        P8SEL0 |= mask0;
        P8SEL1 |= mask1;
        return;
    case IOPORT_J:
        PJSEL0 |= mask0;
        PJSEL1 |= mask1;
        return;
    }
}

void ioout(ioport port, char mask) {
    switch (port) {
    case IOPORT_1:
        P1DIR |= mask;
        return;
    case IOPORT_2:
        P2DIR |= mask;
        return;
    case IOPORT_3:
        P3DIR |= mask;
        return;
    case IOPORT_4:
        P4DIR |= mask;
        return;
    case IOPORT_5:
        P5DIR |= mask;
        return;
    case IOPORT_6:
        P6DIR |= mask;
        return;
    case IOPORT_7:
        P7DIR |= mask;
        return;
    case IOPORT_8:
        P8DIR |= mask;
        return;
    case IOPORT_J:
        PJDIR |= mask;
        return;
    }
}

void iorst(ioport port, char mask) {
    switch (port) {
    case IOPORT_1:
        P1OUT &= ~mask;
        return;
    case IOPORT_2:
        P2OUT &= ~mask;
        return;
    case IOPORT_3:
        P3OUT &= ~mask;
        return;
    case IOPORT_4:
        P4OUT &= ~mask;
        return;
    case IOPORT_5:
        P5OUT &= ~mask;
        return;
    case IOPORT_6:
        P6OUT &= ~mask;
        return;
    case IOPORT_7:
        P7OUT &= ~mask;
        return;
    case IOPORT_8:
        P8OUT &= ~mask;
        return;
    case IOPORT_J:
        PJOUT &= ~mask;
        return;
    }
}

void ioset(ioport port, char mask) {
    switch (port) {
    case IOPORT_1:
        P1OUT |= mask;
        return;
    case IOPORT_2:
        P2OUT |= mask;
        return;
    case IOPORT_3:
        P3OUT |= mask;
        return;
    case IOPORT_4:
        P4OUT |= mask;
        return;
    case IOPORT_5:
        P5OUT |= mask;
        return;
    case IOPORT_6:
        P6OUT |= mask;
        return;
    case IOPORT_7:
        P7OUT |= mask;
        return;
    case IOPORT_8:
        P8OUT |= mask;
        return;
    case IOPORT_J:
        PJOUT |= mask;
        return;
    }
}

void iotog(ioport port, char mask) {
    switch (port) {
    case IOPORT_1:
        P1OUT ^= mask;
        return;
    case IOPORT_2:
        P2OUT ^= mask;
        return;
    case IOPORT_3:
        P3OUT ^= mask;
        return;
    case IOPORT_4:
        P4OUT ^= mask;
        return;
    case IOPORT_5:
        P5OUT ^= mask;
        return;
    case IOPORT_6:
        P6OUT ^= mask;
        return;
    case IOPORT_7:
        P7OUT ^= mask;
        return;
    case IOPORT_8:
        P8OUT ^= mask;
        return;
    case IOPORT_J:
        PJOUT ^= mask;
        return;
    }
}

#pragma vector = PORT2_VECTOR
RAMFUNC interrupt void P2InterruptRoutine(void) {
    if (porthandler[IOPORT_2](P2IN)) P2IES ^= portimask[IOPORT_2];
    P2IFG = 0;
    __low_power_mode_off_on_exit();
}

#pragma vector = PORT4_VECTOR
RAMFUNC interrupt void P4InterruptRoutine(void) {
    if (porthandler[IOPORT_4](P4IN)) P4IES ^= portimask[IOPORT_4];
    P4IFG = 0;
    __low_power_mode_off_on_exit();
}

#pragma vector = PORT5_VECTOR
RAMFUNC interrupt void P5InterruptRoutine(void) {
    if (porthandler[IOPORT_5](P5IN)) P5IES ^= portimask[IOPORT_5];
    P5IFG = 0;
    __low_power_mode_off_on_exit();
}
