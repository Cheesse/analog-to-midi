/*
 * fsg.c
 *
 * Frequency Spectrum Generation
 *  Created on: Aug 14, 2020
 *      Author: Andrew Obeso
 */

/* TODO: Modify LEA DSPLib code so that the 0th FFT bin is replaced with the N/2th bin. */

#include "fsg.h"
#include "midiout.h"
#include "DSPLib.h"
#include <string.h>

#define STATUS_FILTEROUT 0x0001 /* The stage's low-pass filter output can be input into the next stage. */
#define STATUS_TXFORMOUT 0x0002 /* The stage's frequency spectrum was updated and can be aggregated. */
#define STATUS_GENERROR  0x8000 /* Generic error. */

typedef struct {
    unsigned char curBuf;
    const unsigned char pass;
    const unsigned int maxSamples;
    const unsigned int bufSize;
    unsigned int i;
    _q15 *const sampleBuf[STAGE_BUFFERS];
    const msp_fft_q15_params transform;
    const msp_biquad_df1_q15_params filter;
} fsgstage;

/* Stage 0's LEA Memory. */

/* Sample buffers. */
DSPLIB_DATA(stage0buf0, MSP_ALIGN_FFT_Q15(STAGE0_SIZE))
static _q15 stage0buf0[STAGE0_SIZE];
DSPLIB_DATA(stage0buf1, MSP_ALIGN_FFT_Q15(STAGE0_SIZE))
static _q15 stage0buf1[STAGE0_SIZE];

/* IIR filter coefficients. */
DSPLIB_DATA(stage0coeffs, 4)
static msp_biquad_df1_q15_coeffs stage0coeffs;

/* IIR filter states. */
DSPLIB_DATA(stage0fstates, 4)
static msp_biquad_df1_q15_states stage0fstates;

/* Stage 1's LEA Memory. */

/* Sample buffers. */
DSPLIB_DATA(stage1buf0, MSP_ALIGN_FFT_Q15(STAGE1_SIZE))
static _q15 stage1buf0[STAGE1_SIZE];
DSPLIB_DATA(stage1buf1, MSP_ALIGN_FFT_Q15(STAGE1_SIZE))
static _q15 stage1buf1[STAGE1_SIZE];

/* IIR filter coefficients. */
DSPLIB_DATA(stage1coeffs, 4)
static msp_biquad_df1_q15_coeffs stage1coeffs;

/* IIR filter states. */
DSPLIB_DATA(stage1fstates, 4)
static msp_biquad_df1_q15_states stage1fstates;

/* Stage 2's LEA Memory. */

/* Sample buffers. */
DSPLIB_DATA(stage2buf0, MSP_ALIGN_FFT_Q15(STAGE2_SIZE))
static _q15 stage2buf0[STAGE2_SIZE];
DSPLIB_DATA(stage2buf1, MSP_ALIGN_FFT_Q15(STAGE2_SIZE))
static _q15 stage2buf1[STAGE2_SIZE];

/* IIR filter coefficients. */
DSPLIB_DATA(stage2coeffs, 4)
static msp_biquad_df1_q15_coeffs stage2coeffs;

/* IIR filter states. */
DSPLIB_DATA(stage2fstates, 4)
static msp_biquad_df1_q15_states stage2fstates;

/* Stage 3's LEA Memory. */

/* Sample buffers. */
DSPLIB_DATA(stage3buf0, MSP_ALIGN_FFT_Q15(STAGE3_SIZE))
static _q15 stage3buf0[STAGE3_SIZE];
DSPLIB_DATA(stage3buf1, MSP_ALIGN_FFT_Q15(STAGE3_SIZE))
static _q15 stage3buf1[STAGE3_SIZE];

/* No IIR filter because this is the last stage. */

/* IIR filter input and output buffers. Shared among stages. */
#define FILTER_SIZE 2
DSPLIB_DATA(iirout, 4)
static _q15 iirout;

static fsgstage stage[STAGE_COUNT] = {
    { 0, STAGE0_PASS - 1, STAGE0_INT, STAGE0_SIZE, STAGE0_SIZE - STAGE0_INT, { stage0buf0, stage0buf1 },
      { STAGE0_SIZE, true, NULL }, { FILTER_SIZE, &stage0coeffs, &stage0fstates } },
    { 0, STAGE1_PASS - 1, STAGE1_INT, STAGE1_SIZE, STAGE1_SIZE - STAGE1_INT, { stage1buf0, stage1buf1 },
      { STAGE1_SIZE, true, NULL }, { FILTER_SIZE, &stage1coeffs, &stage1fstates } },
    { 0, STAGE2_PASS - 1, STAGE2_INT, STAGE2_SIZE, STAGE2_SIZE - STAGE2_INT, { stage2buf0, stage2buf1 },
      { STAGE2_SIZE, true, NULL }, { FILTER_SIZE, &stage2coeffs, &stage2fstates } },
    { 0, STAGE3_PASS - 1, STAGE3_INT, STAGE3_SIZE, STAGE3_SIZE - STAGE3_INT, { stage3buf0, stage3buf1 },
      { STAGE3_SIZE, true, NULL }, { 0, NULL, NULL } }
};

typedef unsigned char _uq8;

/* The aggregate spectrum the note detection algorithm will use. */
_uq8 spectrum[SPECTRUM_BINS];

/* Processes one stage of the FSG. Returns the status. */
static inline int stagedo(fsgstage *s, _q15 x) {
    int status = 0; msp_status leastatus;

    /* Append sample to sample buffer. */
    s->sampleBuf[s->curBuf][s->i] = x;

    /* Send this sample to the low-pass filter if available. */
    if (s->filter.length && s->i & 1) {
        leastatus = msp_biquad_df1_q15(&s->filter, s->sampleBuf[s->curBuf] + s->i - 1, &iirout);
        msp_checkStatus(leastatus);
        status |= (s->i - 1) & s->pass ? 0 : STATUS_FILTEROUT;
    }

    /* Increment sample index within 0 and bufSize. */
    s->i = (s->i + 1) & (s->bufSize - 1);

    if (!s->i) {
        /* The buffer is filled (or sufficiently filled if maxSamples < bufSize). */
        if (s->maxSamples < s->bufSize) {
            /* Copy the buffer to the next buffer, but in a shifted manner. */
            memcpy(s->sampleBuf[s->curBuf ^ 1u], s->sampleBuf[s->curBuf] + s->maxSamples, s->bufSize - s->maxSamples);
            /*for (i = 0; i != s->bufSize - s->maxSamples; i++)
                s->sampleBuf[s->curBuf ^ 1][i] = s->sampleBuf[s->curBuf][i + s->maxSamples];*/
        }
        /* Now do the FFT. */
        leastatus = msp_fft_fixed_q15(&s->transform, s->sampleBuf[s->curBuf]);
        msp_checkStatus(leastatus);

        /* Swap buffers and reset index. */
        s->curBuf ^= 1;
        s->i = s->bufSize - s->maxSamples;
        status |= STATUS_TXFORMOUT;
    }

    return status;
}

void fsginit(void) {
    /* Initialize stages. All we need to do for now is set coefficients for each filter. */

}

/* Find the magnitude squared of a complex number. Input must be from a sample/spectrum buffer. */
#define COMPLEX_VECTOR_LENGTH 2
DSPLIB_DATA(macout, 4)
static _iq31 macout;
static inline _uq8 abs2(_q15 *cplx) {
    msp_status leastatus;
    static const msp_mac_q15_params p = { COMPLEX_VECTOR_LENGTH };

    leastatus = msp_mac_q15(&p, cplx, cplx, &macout);
    msp_checkStatus(leastatus);

    return (_uq8)((macout >> (24 - 1)) & 0xFF); /* Ignore sign bit since we know the result is always positive. */
}

/* Clamps an index to within the size of an array. */
#define INDEX_CLAMP(i, size) ((i) & ((size) - 1))

/* Naive linear interpolation. */
static inline _uq8 interp(_uq8 left, _uq8 right) {
    return left >> 1 + right >> 1;
}

/* Step frequency is ~6.25 Hz for the aggregate spectrum. */
static inline void transformout(unsigned int stageNum) {
    unsigned int i;
    unsigned int start, count, exp, bufsize;
    fsgstage *s;
    _q15 *buf;

    s = stage + stageNum;
    buf = s->sampleBuf[s->curBuf];
    bufsize = s->bufSize;
    start = stageNum == (STAGE_COUNT - 1) ? 0 : stage[stageNum + 1].bufSize << ((STAGE_COUNT - 3) - stageNum);
    exp = (STAGE_COUNT - 1) - stageNum;
    count = bufsize << ((STAGE_COUNT - 2) - stageNum);

    for (i = count - start; i; i--) {
        if (i & (1 << exp - 1)) {
            spectrum[INDEX_CLAMP(start + i, SPECTRUM_BINS)] = interp(
                spectrum[INDEX_CLAMP((start - 1) + i, SPECTRUM_BINS)],
                abs2(buf + 2 * INDEX_CLAMP((start + i) >> exp + 1, bufsize >> 1))
            );
        } else spectrum[INDEX_CLAMP(start + i, SPECTRUM_BINS)] = abs2(buf + 2 * INDEX_CLAMP((start + i) >> exp, bufsize >> 1));
    }

    /* Show current status of spectrum. */
    P1OUT ^= BIT1;
    for (i = SPECTRUM_BINS; i; i--) {
        midisend(fsgbin(i));
        midiwait();
    }

    /* Pause to allow user to inspect. */
    while (P5IN & BIT6);

    /* Old non-loop version.
    switch (a) {
    case 0: /* 7-gaps
        /* Start from 129 and fill last bins of spectrum (including 0th bin), filling in holes as necessary.
        for (i = (STAGE0_SIZE * 8 - STAGE1_SIZE * 4) / 2; i; i--) {
            if (i & (8 - 1)) {
                /* Fill in the gap.
                spectrum[STAGE1_SIZE * 4 / 2 + i] = interp(spectrum[STAGE1_SIZE * 4 / 2 + i - 1], abs2(sampleBuf + 2 * INDEX_CLAMP(i / 8 + 1, STAGE0_SIZE / 2)));
            } else {
                spectrum[STAGE1_SIZE * 4 / 2 + i] = abs2(sampleBuf + 2 * INDEX_CLAMP(i / 8, STAGE0_SIZE / 2));
            }
        }
        break;
    case 1: /* 3-gaps
        /* Start from 65 and fill next 64 bins of spectrum, filling in holes as necessary.
        for (i = (STAGE1_SIZE * 4 - STAGE2_SIZE * 2) / 2; i; i--) {
            if (i & (4 - 1)) {
                /* Fill in the gap.
                spectrum[STAGE2_SIZE * 2 / 2 + i] = interp(spectrum[STAGE2_SIZE * 2 / 2 + i - 1], abs2(sampleBuf + 2 * INDEX_CLAMP(i / 4 + 1, STAGE1_SIZE / 2)));
            } else {
                spectrum[STAGE2_SIZE * 2 / 2 + i] = abs2(sampleBuf + 2 * INDEX_CLAMP(i / 4, STAGE1_SIZE / 2));
            }
        }
        break;
    case 2: /* 1-gaps
        /* Start from 33 and fill next 32 bins of spectrum, filling in holes as necessary.
        for (i = (STAGE2_SIZE * 2 - STAGE3_SIZE * 1) / 2; i; i--) {
            if (i & (2 - 1)) {
                /* Fill in the gap.
                spectrum[STAGE3_SIZE * 1 / 2 + i] = interp(spectrum[STAGE3_SIZE * 1 / 2 + i - 1], abs2(sampleBuf + 2 * INDEX_CLAMP(i / 2 + 1, STAGE2_SIZE / 2)));
            } else {
                spectrum[STAGE3_SIZE * 1 / 2 + i] = abs2(sampleBuf + 2 * INDEX_CLAMP(i / 2, STAGE2_SIZE / 2));
            }
        }
        break;
    case 3: /* 0-gaps
        /* Start from 1 and fill first 32 bins of spectrum. No need to fill any gaps.
        for (i = (STAGE3_SIZE * 1 - 0 * 0) / 2; i; i--) {
            if (i & (1 - 1)) {
                /* Fill in the gap (no gaps).

            } else {
                spectrum[0 * 0 / 2 + i] = abs2(sampleBuf + 2 * INDEX_CLAMP(i / 1, STAGE3_SIZE / 2));
            }
        }
        break;
    default:
        ;/* error
    }*/
}

void fsgupdate(int x) {
    unsigned int i, status = STATUS_FILTEROUT;

    for (i = (unsigned int)STAGE_COUNT; status & STATUS_FILTEROUT; i--) {
        status = stagedo(stage + ((unsigned int)STAGE_COUNT - i), x);

        if (status & STATUS_TXFORMOUT) transformout((unsigned int)STAGE_COUNT - i);
        x = iirout;
    }
}

_uq8 fsgbin(unsigned int bin) { return spectrum[bin & (SPECTRUM_BINS - 1)]; }
