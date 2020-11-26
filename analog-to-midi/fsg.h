/*
 * fsg.h
 *
 * Frequency Spectrum Generator
 *  Created on: Sep 13, 2020
 *      Author: Andrew Obeso
 */

#ifndef FSG_H_
#define FSG_H_

/* RMS of the current frame's waveform. */
extern unsigned int framerms;

/* Frequency spectra for each stage. */
extern unsigned char topspec[FSG_TOPSTAGE_SIZE / 2];
extern unsigned char subspec[FSG_SUBSTAGE_COUNT][FSG_SUBSTAGE_SIZE / 2];

#if DEBUG_FSG_PRINT == 1
/* Returns the pointer to the substage buffer. */
inline unsigned char* fsggetsub(unsigned int substage);

/* Returns the pointer to the top stage buffer. */
inline unsigned char* fsggettop(void);
#endif /* DEBUG_FSG_PRINT == 1 */

/* Passes the top stage sample buffer through the IIR filters to the substages. Call this before calling fsgregen. */
FASTFUNC void fsgprep(void);

/* Regenerates the spectra of each stage. Called after a return from fsgprep. */
FASTFUNC void fsgregen(void);

/* Takes an ADC sample as input and puts it in the topmost buffer. Updates the buffers of lower stages as appropriate. */
RAMFUNC int fsgsync(int x);

/* Waits until the spectra are ready to be regenerated.  Call fsgprep after returning from this function. */
inline void fsgwait(void);

#endif /* FSG_H_ */
