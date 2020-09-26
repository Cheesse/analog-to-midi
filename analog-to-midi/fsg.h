/*
 * fsg.h
 *
 * Frequency Spectrum Generator
 *  Created on: Sep 13, 2020
 *      Author: Andrew Obeso
 */

#ifndef FSG_H_
#define FSG_H_

#define TOPSTAGE_SIZE 512

#define SUBSTAGE_COUNT 2
#define SUBSTAGE_SIZE 64
#define SUBSTAGE1_INT 32
#define SUBSTAGE0_INT 16

/* Magnitude-squared frequency spectra for each stage. */
extern unsigned char topspec[TOPSTAGE_SIZE / 2];
extern unsigned char subspec[SUBSTAGE_COUNT][SUBSTAGE_SIZE / 2];

/* Initializes the Frequency Spectrum Generator. */
void fsginit(void);

/* Regenerates the spectra of each stage. Called after a return from fsgwait(). */
void fsgregen(void);

/* Takes an ADC sample as input and puts it in the topmost buffer. Updates the buffers of lower stages as appropriate. */
inline void fsgupdate(int x);

/* Waits until an FFT can be done. With the current configuration, all 3 stages will have their spectra regenerated at once. */
void fsgwait(void);

#endif /* FSG_H_ */
