/*
 * detection.h
 *
 * Note Detection
 *  Created on: Nov 24, 2020
 *      Author: Noah Watts
 */

#ifndef DETECTION_H_
#define DETECTION_H_

extern char output[6][3];

/* passes the fft spectrums to the note detection, might be better to change to pass a pointer instead*/
void setSpectrum(unsigned char *topSpec, unsigned char *midSpec, unsigned char *lowSpec);

/* Runs the computation in the note detection so output is ready whenever needed */
void generateOutput(void);

/* returns the output in the form of a pointer to a char[6][3] array, char[i][0] is midi value, char[i][1] is magnitude char[i][2] is used in the algorithm but not relevant for output */
char** getOutput(void);

#endif /* DETECTION_H_ */
