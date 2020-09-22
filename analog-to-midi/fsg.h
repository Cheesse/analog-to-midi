/*
 * fsg.h
 *
 * Frequency Spectrum Generator
 *  Created on: Sep 13, 2020
 *      Author: Andrew Obeso
 */

#ifndef FSG_H_
#define FSG_H_

#define SPECTRUM_BINS 2048

#define STAGE_COUNT   4
#define STAGE_BUFFERS 2

#define STAGE0_SIZE 512
#define STAGE0_INT  512
#define STAGE0_PASS  16
#define STAGE1_SIZE  64
#define STAGE1_INT   64
#define STAGE1_PASS   2
#define STAGE2_SIZE  64
#define STAGE2_INT   32
#define STAGE2_PASS   2
#define STAGE3_SIZE  64
#define STAGE3_INT   16
#define STAGE3_PASS   1

/* Initializes the Frequency Spectrum Generator. */
void fsginit(void);

/* Takes an ADC sample as input. */
void fsgupdate(int x);

/* Gets the magnitude^2 of a frequency bin, from 1 to N/2, of the generated spectrum. */
unsigned char fsgbin(unsigned int bin);

#endif /* FSG_H_ */
