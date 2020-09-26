/*
 * adcd.c
 *
 * Analog-to-Digital Converter Driver
 *  Created on: Aug 14, 2020
 *      Author: Andrew Obeso
 */

#include <msp430.h>
#include "adcd.h"
#include "fsg.h"

static volatile int wait = 0;

void adcinit(void) {
    /* Using ADC Memory Register 0 to store conversion results. */

    /* CHANGE THIS WHEN CHANGING ADC CHANNEL. */
    /* Enable ADC input on Port 1.0. */
    P1SEL0 |= BIT0;
    P1SEL1 |= BIT0;

    /* ADC12MCTL0: Memory Control Register for ADC Memory Register 0.
     *  ADC12WINC  (Comparator Window)  = 0 (Disabled)
     *  ADC12DIF   (Differential Mode)  = 0 (Single-ended mode)
     *  ADC12VRSEL (Voltage References) = 0 (VR+ = AVCC; VR- = AVSS)
     *  ADC12EOS   (End of Sequence)    = 1 (End of sequence)
     *  ADC12INCH  (Input Channel)      = ADC_CHANNEL */
    ADC12MCTL0 = ADC12WINC_0 | ADC12DIF_0 | ADC12VRSEL_0 | ADC12EOS_1 | ADC_CHANNEL;

    /* Set ADC Interrupt Enable Register 0. Enable only the flag for Memory Register 0. */
    ADC12IER0 = ADC12IE0;

    /* ADC12CTL3: ADC Control Register 3.
     *  ADC12ICH3MAP   (Internal Channel 3 Select)  = X (Don't care)
     *  ADC12ICH2MAP   (Internal Channel 2 Select)  = X (Don't care)
     *  ADC12ICH1MAP   (Internal Channel 1 Select)  = X (Don't care)
     *  ADC12ICH0MAP   (Internal Channel 0 Select)  = X (Don't care)
     *  ADC12TCMAP     (Temperature Sensor Channel) = X (Don't care)
     *  ADC12BATMAP    (1/2 AVCC Channel Select)    = X (Don't care)
     *  ADC12CSTARTADD (Target Memory Register)     = 0 */
    ADC12CTL3 = ADC12CSTARTADD_0;

    /* ADC12CTL2: ADC Control Register 2.
     *  ADC12RES (Resolution)   = 0 (8-bit)
     *  ADC12DF  (Data Format)  = 1 (Signed left-aligned)
     *  ADC12PWRMD (Power Mode) = 0 (Normal) */
    ADC12CTL2 = ADC12RES_0 | ADC12DF_1 | ADC12PWRMD_0;

    /* ADC12CTL1: ADC Control Register 1.
     *  ADC12PDIV   (Predivider)      = 0 (Divide by 1)
     *  ADC12SHS    (Trigger Source)  = 0 (ADC12SC)
     *  ADC12SHP    (Pulse-mode)      = 1 (Sampling-timer)
     *  ADC12ISSH   (Invert Trigger)  = 0 (No)
     *  ADC12DIV    (Clock Divider)   = 0 (Divide by 1)
     *  ADC12SSEL   (Clock Source)    = 0 (ADC12OSC)
     *  ADC12CONSEQ (Conversion Mode) = 0 (1-channel, once) */
    ADC12CTL1 = ADC12PDIV_0 | ADC12SHS_0 | ADC12SHP_1 | ADC12ISSH_0 | ADC12DIV_0 | ADC12SSEL_0 | ADC12CONSEQ_0;

    /* ADC12CTL0: ADC Control Register 0.
     *  ADC12SHT1 (Sample Time 8-23)   = X (Don't care)
     *  ADC12SHT0 (Sample Time Others) = 0 (4 cycles)
     *  ADC12MSC  (Multisample Mode)   = X (Don't care)
     *  ADC12ON   (ADC On)             = 1 (Yes)
     *  ADC12ENC  (Enable Conversion)  = 1 (Yes)
     *  ADC12SC   (Start Conversion)   = 0 (No) */
    ADC12CTL0 = ADC12SHT0_0 | ADC12ON_1 | ADC12ENC_1 | ADC12SC_0;

    /* ADC enabled and conversion begun. */
}

/* Commands the ADC to take a sample. */
int adcsample(void) {
    static int x = 0x0000;
    /* Save interrupt state. */
    //unsigned int istate = __get_interrupt_state();

    /* Collect a sample and wait. */
    //ADC12CTL0 |= ADC12SC;
    //wait = 1;
    //while (wait);
    x ^= 0x7F00;
    return x >> 2;
    //__low_power_mode_0();

    /* Restore interrupt state. */
    //__set_interrupt_state(istate);
    //return ADC12MEM0;
}

#pragma vector = ADC12_B_VECTOR
interrupt void ADCInterruptRoutine(void) {
    /* Wake the processor to start processing the sample. */
    //__low_power_mode_off_on_exit();
    wait = 0;
    fsgupdate(ADC12MEM0);
    /* Clear flag to exit. */
    ADC12IFGR0 &= ~ADC12IFG0;
}
