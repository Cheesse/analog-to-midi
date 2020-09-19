/*
 * adcd.h
 *
 * Analog-to-Digital Converter Driver
 *  Created on: Sep 13, 2020
 *      Author: Andrew Obeso
 */

#ifndef ADCD_H_
#define ADCD_H_

#define SAMPLE_RATE 25600

typedef unsigned int sample;

/* Initializes the ADC driver. */
void adcinit(void);

#endif /* ADCD_H_ */
