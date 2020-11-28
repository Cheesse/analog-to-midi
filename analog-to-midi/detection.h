/*
 * detection.h
 *
 * Note Detection
 *  Created on: Nov 24, 2020
 *      Author: Noah Watts
 */

#ifndef DETECTION_H_
#define DETECTION_H_

extern char output[MIDI_POLYPHONY][3];
extern uint32_t HPS[88];

/* Passes the FFT spectra to the note detection. */
void setSpectrum(unsigned char *topSpec, unsigned char *midSpec, unsigned char *lowSpec);

/* Runs the computation in the note detection so output is ready whenever needed. */
void generateOutput(void);

#endif /* DETECTION_H_ */
