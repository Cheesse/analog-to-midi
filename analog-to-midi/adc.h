/*
 * adc.h
 *
 * Analog-to-Digital Converter Driver
 *  Created on: Sep 13, 2020
 *      Author: Andrew Obeso
 */

/* NOTE: Uses ADC12_B, Timer A0.0, Timer A0.1, and REF_A. */

#ifndef ADC_H_
#define ADC_H_

#define ADC_SEL0 IO_ADC_PIN
#define ADC_SEL1 IO_ADC_PIN

#define ADC_REF_PORT IOPORT_1
#define ADC_REF_PIN PIN1
#define ADC_REF_SEL0 ADC_REF_PIN
#define ADC_REF_SEL1 ADC_REF_PIN

/* Function to be called by the ADC after a sample is collected. The return value determines if the CPU will wake. */
typedef int (*adcfunc)(int sample);

#if DEBUG_FSG_PRINT == 1
/* Disables the ADC. */
inline void adcdisable(void);
#endif /* DEBUG_FSG_PRINT == 1 */

/* Enables the ADC. */
inline void adcenable(void);

/* Initializes the ADC driver with a callback function that is called after each sample. */
inline void adcinit(adcfunc callback);

#endif /* ADC_H_ */
