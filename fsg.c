/*
 * fsg.c
 *
 * Frequency Spectrum Generation
 *  Created on: Aug 14, 2020
 *      Author: Andrew Obeso
 */

#include "fsg.h"
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
DSPLIB_DATA(iirin, 4)
static _q15 iirin;
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
        iirin = x;
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
            memcpy(s->sampleBuf[s->curBuf ^ 1], s->sampleBuf[s->curBuf] + s->maxSamples, s->bufSize - s->maxSamples);
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

void fsgupdate(int x) {
    unsigned int i, status = STATUS_FILTEROUT;

    for (i = 0; status & STATUS_FILTEROUT; i++) {
        status = stagedo(stage + i, x);

        if (status & STATUS_TXFORMOUT) {
            // do transform out
            // Also make sure to fill in gaps
        }
        x = iirout;
    }
}
