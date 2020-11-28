/*
 * main.c
 *
 * Analog-to-MIDI Converter MAIN
 *  Created on: Sep 18, 2020
 *      Author: andob
 */

//#define PRINT_SAMPLE_BUFFERS

#include "config.h"
#include "defs.h"

#include "adc.h"
#include "cpu.h"
#include "detection.h"
#include "duart.h"
#include "fsg.h"
#include "io.h"
#include "midi.h"
#include "uat.h"

#if DEBUG_FSG_PRINT == 1
static volatile int wait = 0;

FASTFUNC static void printbuffers(void) {
    int y;
    unsigned int i, j;
    int *b;

    b = fsggettop();

    /* Show current status of top buffer. */
    for (i = 0; i != FSG_TOPSTAGE_SIZE; i++) {
        y = b[i];
        if (y < 0) duartoutc((unsigned char)(((y + 0x80) & 0xFF00) >> 8));
        else duartoutc((unsigned char)((y & 0xFF00) >> 8));
        duartwait();
    }

    /* Show current status of every substage's buffer. */
    i = FSG_SUBSTAGE_COUNT;
    while (i--) {
        b = fsggetsub(i);
        for (j = 0; j != FSG_SUBSTAGE_SIZE; j++) {
            y = b[j];
            if (y < 0) duartoutc((unsigned char)(((y + 0x80) & 0xFF00) >> 8));
            else duartoutc((unsigned char)((y & 0xFF00) >> 8));
            duartwait();
        }
    }
}

RAMFUNC static int handleContSignal(char in) {
    wait = 0;
    return 0;
}
#elif DEBUG_FSG_PRINT == 2
FASTFUNC static void printspectra(void) {
    unsigned int i;
    unsigned char *b;

    /* Show current status of top spectrum. */
    duartouts((char*)topspec, FSG_TOPSTAGE_SIZE / 2);

    /* Show current status of every substage's spectrum. */
    i = FSG_SUBSTAGE_COUNT;
    while (i--) {
        b = subspec[i];
        duartouts((char*)b, FSG_SUBSTAGE_SIZE / 2);
    }
}
#endif /* DEBUG_FSG_PRINT == 2 */

/* Receive the UAT output and send out it's inverse. */
RAMFUNC static int handleUatLoopback(char in) {
    /* Output inverted signal. Just need to toggle the output. */
    iotog(IO_UAT_INVERT_PORT, IO_UAT_INVERT_PIN);

    return 1;
}

void main(void) {
    cpuinit();

    ioinit();

    /* Init ADC input. */
    iomode(ADC_REF_PORT, ADC_REF_SEL0, ADC_REF_SEL1);
    iomode(IO_ADC_PORT, ADC_SEL0, ADC_SEL1);

    /* Init Debug UART output. */
    iomode(DEBUG_UART_PORT, UART_SEL0, UART_SEL1);

    /* Init UAT output. */
    iomode(IO_UAT_PORT, UAT_SEL0, UAT_SEL1);

    /* Init UAT loopback. */
    ioin(IO_UAT_LOOPBACK_PORT, IO_UAT_LOOPBACK_PIN, 0);
    iohandler(IO_UAT_LOOPBACK_PORT, IO_UAT_LOOPBACK_PIN, 0, handleUatLoopback);
    ioset(IO_UAT_INVERT_PORT, IO_UAT_INVERT_PIN);

#if DEBUG_FSG_PRINT == 1
    ioin(DEBUG_FSG_CONT_PORT, DEBUG_FSG_CONT_PIN, DEBUG_FSG_CONT_PIN);
    iohandler(DEBUG_FSG_CONT_PORT, DEBUG_FSG_CONT_PIN, DEBUG_FSG_CONT_PIN, handleContSignal);
    ioset(DEBUG_FSG_CONT_PORT, DEBUG_FSG_CONT_PIN);
#endif /* DEBUG_FSG_PRINT == 1 */

    adcinit(&fsgsync);

#if DEBUG_FSG_PRINT
    duartinit();
#endif /* DEBUG_FSG_PRINT */

    uatinit();

    ioenable();
    adcenable();

#if DEBUG_FSG_PRINT
    duartenable();
#endif /* DEBUG_FSG_PRINT */

    uatenable();

    midiinit();

    for (;;) {
#if !DEBUG_MIDI
        /* Wait until sample buffers are full. */
        fsgwait();

#if DEBUG_FSG_PRINT == 1
        adcdisable();
#endif /* DEBUG_FSG_PRINT == 1 */

#if DEBUG_FSG_PERIOD
        iotog(DEBUG_FSG_PERIOD_PORT, DEBUG_FSG_PERIOD_PIN);
#endif /* DEBUG_FSG */

        /* Prepare the substages. */
        fsgprep();

#if DEBUG_FSG_PRINT == 1
        //printbuffers();
#endif /* DEBUG_FSG_PRINT == 1 */

        /* Regenerate the frequency spectra. */
        fsgregen();

#if DEBUG_FSG_PERIOD
        iotog(DEBUG_FSG_PERIOD_PORT, DEBUG_FSG_PERIOD_PIN);
#endif /* DEBUG_FSG */

        /* Try to detect notes. */
        //uatouts("\x3C\x7F", 2);
        setSpectrum(topspec, subspec[1], subspec[0]);
        generateOutput();
        midigen();
        midiout();

#if DEBUG_FSG_PRINT == 1
        //printbuffers();
        duartouts(output[0], 3);
        duartouts(output[1], 3);
        duartouts(output[2], 3);
        duartouts(output[3], 3);
        duartouts(output[4], 3);
        duartouts(output[5], 3);
#endif /* DEBUG_FSG_PRINT == 1 */

#if DEBUG_FSG_PRINT == 1
        /* Pause to allow user to inspect. */
        wait = 1;
        cpuwait(&wait);

        /* Reenable ADC. */
        adcenable();
#elif DEBUG_FSG_PRINT == 2
        printspectra();
#endif /* DEBUG_FSG_PRINT */

#if DEBUG_ITER_PERIOD
        iotog(DEBUG_ITER_PERIOD_PORT, DEBUG_ITER_PERIOD_PIN);
#endif /* DEBUG_ITER_PERIOD */

#else
        unsigned long i;
        ioset(DEBUG_MIDI_PORT, DEBUG_MIDI_PIN);

        uatouts("\x3C\x7F", 2);

        iorst(DEBUG_MIDI_PORT, DEBUG_MIDI_PIN);
        for (i = 8000000; i; i--);
        ioset(DEBUG_MIDI_PORT, DEBUG_MIDI_PIN);

        uatouts("\x3C\x00", 2);

        iorst(DEBUG_MIDI_PORT, DEBUG_MIDI_PIN);
        for (i = 8000000; i; i--);
#endif /* DEBUG_MIDI */
    }
}
