/*
 * cpu.c
 *
 * CPU Driver
 *  Created on: Nov 17, 2020
 *      Author: andob
 */

#include "config.h"
#include "defs.h"

#include "cpu.h"

#include <msp430.h>

void cpuinit(void) {
    /* Stop WDT. */
    WDTCTL = WDTPW | WDTHOLD;

    /* Set FRAM to have 1 wait state in preparation for the increased clock speed. */
    FRCTL0_L |= NWAITS1;

    /* MCLK is sourced from DCOSC. SMCLK is sourced from DCOSC or MODOSC. */

    /* Increase MCU clock rate from 8 MHz to 16 MHz. */
    CSCTL0 = CSKEY; /* Unlock CS registers. */
    CSCTL1 = DCORSEL | DCOFSEL_4; /* Increase DCOSC clock speed to 16 MHz. */
#if SMCLK_USE_DCOSC
    //CSCTL2 = SELS_3 | SELM_3; /* Select DCOSC for SMCLK and DCOSC for MCLK. */
#else
    CSCTL2 = SELS_4 | SELM_3; /* Select MODOSC for SMCLK and DCOSC for MCLK. */
#endif /*SMCLK_USE_DCOSC */
    CSCTL3 = (SMCLK_DIV_EXP << 4) | DIVM_0; /* Set MCLK divider to 1 (16 MHz) and SMCLK divider. Don't care about ACLK. */
    CSCTL0_H = 0; /* Lock CS registers. */
}

void cpuwait(volatile int *ptr) {
    if (ptr) while (*ptr) __low_power_mode_0();
    else __low_power_mode_0();
}
