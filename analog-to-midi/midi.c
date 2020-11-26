/*
 * midi.c
 *
 * MIDI Data Stream Generator
 *  Created on: Aug 14, 2020
 *      Author: Andrew Obeso
 */

#include "config.h"
#include "defs.h"

#include "detection.h"
#include "midi.h"
#include "uat.h"

/* Number of characters to send. */
static unsigned int chars = 0;

/* Bitfields of active and inactive notes. */
static unsigned char notes[2][MIDI_NOTE_COUNT / 8];

/* MIDI output buffer. */
static char buf[MIDI_POLYPHONY * 2 * 2];

/* Current bitfield used to hold old notes. */
static unsigned char curbf = 0;

#define newnotes (notes[curbf ^ 1])
#define oldnotes (notes[curbf])

/* Clears all bits in a bitfield. */
#define bfclr(bfptr) i = MIDI_NOTE_COUNT / 8 / 2; while (i--) ((unsigned int*)bfptr)[i] = 0;

/* Gets a bit in a bitfield. */
#define bfget(bfptr, bitnum) ((bfptr)[(bitnum) >> 3] & (1 << ((bitnum) & (8 - 1))))

/* Resets a bit in a bitfield. */
#define bfrst(bfptr, bitnum) ((bfptr)[(bitnum) >> 3] &= ~(1 << ((bitnum) & (8 - 1))))

/* Sets a bit in a bitfield. */
#define bfset(bfptr, bitnum) ((bfptr)[(bitnum) >> 3] |= (1 << ((bitnum) & (8 - 1))))

/* Sends a Note On message for the given note. */
#define send(note, vel) buf[chars++] = ((unsigned char)note); buf[chars++] = (vel)

void midigen(const char *const *data) {
    unsigned int i;
    unsigned char a;

    /* Update the new notes bitfield. */
    i = MIDI_POLYPHONY;
    while (i--) {
        a = output[i][0];
        if (a) bfset(newnotes, a);
    }

    /* Go through each note and turn on or off each one depending on change. */
    i = MIDI_NOTE_COUNT;
    while (i--) {
        a = bfget(newnotes, i);
        if (a ^ bfget(oldnotes, i)) {
            if (a) { send(i, 0x7F); }
            else { send(i, 0x00); }
        }
    }

    /* Clear the old notes bitfield and swap. */
    bfclr(oldnotes);
    curbf ^= 1;
}

void midiinit(void) {
    /* Send the Note On status byte through the UAT since that's the only kind of message we will send. */
    /* Don't need to do this through SPI since USB controller assumes this message will always be sent. */
    uatoutc(0x9C);
}

void midiout(void) {
    if (chars) {
        uatouts(buf, chars);
        //spiouts(buf, chars);

        chars = 0;
    }
}
