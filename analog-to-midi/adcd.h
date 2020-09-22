/*
 * adcd.h
 *
 * Analog-to-Digital Converter Driver
 *  Created on: Sep 13, 2020
 *      Author: Andrew Obeso
 */

#ifndef ADCD_H_
#define ADCD_H_

#define ADC_CHANNEL 0x00
#define SAMPLE_RATE 25600

/* Initializes the ADC driver. */
void adcinit(void);

/* Collects a sample. */
int adcsample(void);

#endif /* ADCD_H_ */
