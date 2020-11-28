/*
 * midi.h
 *
 * MIDI Data Stream Generator
 *  Created on: Sep 21, 2020
 *      Author: andob
 */

#ifndef MIDI_H_
#define MIDI_H_

#define MIDI_NOTE_COUNT 128

/* Generates the next chunk of data based on the given data from the note detection algorithm. */
FASTFUNC void midigen(void);

/* Initializes the MIDI stream generator. */
inline void midiinit(void);

/* Sends the generated MIDI stream through the UAT and/or SPI. */
FASTFUNC void midiout(void);

#endif /* MIDI_H_ */
