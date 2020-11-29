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

/* State machines of active and inactive notes. */
static unsigned char notes[2][MIDI_NOTE_COUNT];

/* MIDI output buffer. */
static char buf[MIDI_POLYPHONY * 2 * 2 * 2];

/* Current bitfield used to hold old notes. */
static unsigned char curbf = 0;

#define newnotes (notes[1])
#define oldnotes (notes[0])

/* Clears all bits in a state machine. */
#define smclr(bfptr) i = MIDI_NOTE_COUNT; while (i--) (bfptr)[i] = 0

#define smcpy() i = MIDI_NOTE_COUNT; while (i--) notes[0][i] = notes[1][i]

/* Decrements a bit in a state machine. */
#define smdec(bfptr, note) ((bfptr)[(note)] -= ((bfptr)[(note)] ? 1 : 0))

/* Gets a bit in a state machine. */
#define smget(bfptr, note) ((bfptr)[(note)])

/* Sets a bit in a state machine. */
#define sminc(bfptr, note) ((bfptr)[(note)] += ((bfptr)[(note)] != 3 ? 1 : 0))

/* Sends a Note On message for the given note. */
#define send(note, vel) buf[chars++] = ((unsigned char)note); buf[chars++] = (vel)

void midigen(void) {
    unsigned int i, j;
    unsigned char a, b;

    /* Update the new notes state machines. */
    i = MIDI_NOTE_COUNT;
    while (i--) {
        /* For this note, scan the output buffer to see if it's in there. */
        j = MIDI_POLYPHONY;
        while (j--) {
            a = output[j][0];
            if ((a == i) && a) {
                uatoutc(a);
                uatoutc(0x7F);
                /* Increment the state if the note is there. */
                //sminc(newnotes, i);
                if (newnotes[i] != 3) newnotes[i]++;
                goto next;
            }
        }

        //smdec(newnotes, i);
        if (newnotes[i]) newnotes[i]--;
next:
        continue;
    }

    /* Go through each note and turn on or off each one depending on change. */
    i = MIDI_NOTE_COUNT;
    while (i--) {
        a = smget(newnotes, i);
        b = smget(oldnotes, i);
        //if (!a && b || a && !b) {
            if (a) { send(i, 0x7F); }
            else { send(i, 0x00); }
        //}
    }

    /* Clear the old notes bitfield and swap. */
    //smclr(oldnotes);
    smcpy();
    //curbf ^= 1;
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
