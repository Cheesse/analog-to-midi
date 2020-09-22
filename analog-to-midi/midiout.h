/*
 * midiout.h
 * MIDI OUTput
 *
 *  Created on: Sep 21, 2020
 *      Author: andob
 */

#ifndef MIDIOUT_H_
#define MIDIOUT_H_

void midiinit(void);

/* Queues 1 character to be sent on the UART stream. */
int midisend(char c);

/* Waits until a character is sent by UART. */
void midiwait(void);

#endif /* MIDIOUT_H_ */
