/*
 * adc.c
 *
 * Analog-to-Digital Converter Driver
 *  Created on: Aug 14, 2020
 *      Author: Andrew Obeso
 */

#include "config.h"
#include "defs.h"

#include "adc.h"
#include "cpu.h"
#if DEBUG_ADC_PERIOD
#include "io.h"
#endif /* DEBUG_ADC_PERIOD */

#include <msp430.h>

/* This used to compile correctly only with small memory model, but now it compiles fine on large model. */
static adcfunc samplecallback;

#if DEBUG_FSG_PRINT == 1
void adcdisable(void) {
    /* Disable ADC. */
    ADC12CTL0 &= ~ADC12ENC;

    /* Disable timer. */
    TA0CTL &= ~MC;
}
#endif /* DEBUG_FSG_PRINT == 1 */

void adcenable(void) {
    /* Enable timer. */
    TA0CTL |= MC_1;

    /* Enable ADC. */
    ADC12CTL0 |= ADC12ENC;
}

void adcinit(adcfunc callback) {
    /* Using ADC Memory Register 0 to store conversion results. */

    /* First set up the sample-and-hold timer. We can use the TA0 CCR1 timer. */

    /* TA0CCR1: Timer A Capture/Compare Register 1. */
    /* Set the period of this timer to be half that of the sampling period. */
    TA0CCR1 = (SMCLK_FREQ / ADC_SAMPLE_RATE / 2 + (SMCLK_FREQ % (ADC_SAMPLE_RATE / 2) >= (ADC_SAMPLE_RATE / 4) ? 1 : 0)) - 1;

    /* TA0CCR0: Timer A Capture/Compare Register 0. */
    TA0CCR0 = TA0CCR1;

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
    TA0CCTL1 = CM_0 | CCIS_0 | CAP_0 | OUTMOD_4 | CCIE_0;

    /* TA0CTL: Timer A0 Control Register.
     *  TASSEL (Clock Source Select) = 2 (SMCLK)
     *  ID     (Input Clock Divider) = 0 (1)
     *  MC     (Mode Control)        = 0 (Stop mode)
     *  TACLR  (Clear Timer)         = X (Don't care)
     *  TAIE   (Interrupt Enable)    = 0 (No)
     *  TAIFG  (Interrupt Flag)      = X (Don't care) */
    TA0CTL = TASSEL_2 | ID_0 | MC_0 | TAIE_0;

    /* Then set up the voltage reference output to output 1.2 V for signal biasing to ADC range. */

    /* REFCTL0: Voltage Reference A Control Register.
     *  REFBGOT  (Bandgap Trigger)     = X (Don't care)
     *  REFGENOT (Reference Generate)  = X (Don't care)
     *  REFVSEL  (Voltage Select)      = 0 (1.2 V)
     *  REFTCOFF (Temp. Sense Disable) = 1 (Disable)
     *  REFOUT   (Output Reference)    = 1 (Yes)
     *  REFON    (Reference Enable)    = 1 (Yes)
     */
    REFCTL0_L = REFVSEL_0 | REFTCOFF_1 | REFOUT_1 | REFON_1;

    /* Finally set up the ADC. */

    /* Set the callback. */
    samplecallback = callback;

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
     *  ADC12RES (Resolution)   = 2 (12-bit)
     *  ADC12DF  (Data Format)  = 1 (Signed left-aligned)
     *  ADC12PWRMD (Power Mode) = 1 (Low power) */
    ADC12CTL2 = ADC12RES_2 | ADC12DF_1 | ADC12PWRMD_1;

    /* ADC12CTL1: ADC Control Register 1.
     *  ADC12PDIV   (Predivider)      = 0 (Divide by 1)
     *  ADC12SHS    (Trigger Source)  = 1 (Timer A 0.1)
     *  ADC12SHP    (Pulse-mode)      = 1 (Sample timer signal)
     *  ADC12ISSH   (Invert Trigger)  = 0 (No)
     *  ADC12DIV    (Clock Divider)   = 1 (Divide by 2)
     *  ADC12SSEL   (Clock Source)    = 2 (SMCLK)
     *  ADC12CONSEQ (Conversion Mode) = 2 (1-channel, repeat) */
    ADC12CTL1 = ADC12PDIV_0 | ADC12SHS_1 | ADC12SHP_1 | ADC12ISSH_0 | ADC12DIV_1 | ADC12SSEL_2 | ADC12CONSEQ_2;

    /* ADC12CTL0: ADC Control Register 0.
     *  ADC12SHT1 (Sample Time 8-23)   = X (Don't care)
     *  ADC12SHT0 (Sample Time Others) = X (Don't care)
     *  ADC12MSC  (Multisample Mode)   = 0 (Sample each edge)
     *  ADC12ON   (ADC On)             = 1 (Yes)
     *  ADC12ENC  (Enable Conversion)  = 0 (No)
     *  ADC12SC   (Start Conversion)   = 0 (No) */
    ADC12CTL0 = ADC12MSC_0 | ADC12ON_1 | ADC12ENC_0 | ADC12SC_0;
}

#pragma vector = ADC12_B_VECTOR
RAMFUNC interrupt void ADCInterruptRoutine(void) {
#if DEBUG_ADC_PERIOD
    iotog(DEBUG_ADC_PERIOD_PORT, DEBUG_ADC_PERIOD_PIN);
#endif /* DEBUG_ADC_PERIOD */
    if (samplecallback(ADC12MEM0)) __low_power_mode_off_on_exit();
}
