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

typedef unsigned char _uq8;

/* FSG Top Stage */
typedef struct {
    unsigned int cur;
    unsigned int i;
    _q15 *buf[2];
    const msp_fft_q15_params txfm;
} fsgtop;

/* FSG Substage */
typedef struct {
    unsigned int cur;
    const unsigned int max;
    _q15 *buf[2];
    const msp_biquad_df1_q15_params iir;
} fsgsub;

/* Top Stage's LEA Memory. */

/* Sample buffers. */
DSPLIB_DATA(topbuf0, MSP_ALIGN_FFT_Q15(TOPSTAGE_SIZE))
static _q15 topbuf0[TOPSTAGE_SIZE];
DSPLIB_DATA(topbuf1, MSP_ALIGN_FFT_Q15(TOPSTAGE_SIZE))
static _q15 topbuf1[TOPSTAGE_SIZE];

/* Substage 1's LEA Memory. */

/* Sample buffers. */
DSPLIB_DATA(sub1buf0, MSP_ALIGN_FFT_Q15(SUBSTAGE_SIZE))
static _q15 sub1buf0[SUBSTAGE_SIZE];
DSPLIB_DATA(sub1buf1, MSP_ALIGN_FFT_Q15(SUBSTAGE_SIZE))
static _q15 sub1buf1[SUBSTAGE_SIZE];

/* IIR filter coefficients. */
DSPLIB_DATA(sub1fc, 4)
static const msp_biquad_df1_q15_coeffs sub1fc = { };

/* IIR filter states. */
DSPLIB_DATA(sub1fs, 4)
static msp_biquad_df1_q15_states sub1fs;

/* Stage 0's LEA Memory. */

/* Sample buffers. */
DSPLIB_DATA(sub0buf0, MSP_ALIGN_FFT_Q15(SUBSTAGE_SIZE))
static _q15 sub0buf0[SUBSTAGE_SIZE];
DSPLIB_DATA(sub0buf1, MSP_ALIGN_FFT_Q15(SUBSTAGE_SIZE))
static _q15 sub0buf1[SUBSTAGE_SIZE];

/* IIR filter coefficients. */
DSPLIB_DATA(sub0fc, 4)
static const msp_biquad_df1_q15_coeffs sub0fc = { };

/* IIR filter states. */
DSPLIB_DATA(sub0fs, 4)
static msp_biquad_df1_q15_states sub0fs;

/* IIR filter output buffer. Shared among substages. */
DSPLIB_DATA(iirout, TOPSTAGE_SIZE / 2* 2)
static _q15 iirout[TOPSTAGE_SIZE / 2];

/* Top stage. */
static fsgtop top = { 0, 0, { topbuf0, topbuf1 }, { TOPSTAGE_SIZE, true } };

/* Substages. */
static fsgsub sub[SUBSTAGE_COUNT] = {
    { 0, SUBSTAGE0_INT, { sub0buf0, sub0buf1 }, { SUBSTAGE_SIZE, &sub0fc, &sub0fs } },
    { 0, SUBSTAGE1_INT, { sub1buf0, sub1buf1 }, { TOPSTAGE_SIZE, &sub1fc, &sub1fs } }
};

/* Substage FFT parameters. */
static const msp_fft_q15_params subtxfm = { SUBSTAGE_SIZE, true };

/* Stage magnitude-squared spectra. */
unsigned char topspec[TOPSTAGE_SIZE / 2];
unsigned char subspec[SUBSTAGE_COUNT][SUBSTAGE_SIZE / 2];

static volatile int wait;

/* Find the magnitude squared of a complex number. Input must be from a sample/spectrum buffer. In-place operation. */
static inline _uq8 abs2(_q15 *cplx) {
    msp_status leastatus;
    _iq31 *macout = (_iq31*)cplx;
    static const msp_mac_q15_params p = { 2 };

    leastatus = msp_mac_q15(&p, cplx, cplx, macout);
    msp_checkStatus(leastatus);

    return (_uq8)((*macout >> (24 - 1)) & 0xFF); /* Ignore sign bit since we know the result is always positive. */
}

/* Calculate the average of several numbers. */
static inline _q15 avg(_q15 *a, unsigned int ctexp) {
    unsigned int count;
    _q15 sum = 0;

    count = 1 << ctexp;
    while (count) sum += a[--count] >> ctexp;

    return sum;
}

/* Regenerates a substage's magnitude-squared frequency spectrum. */
static inline void subregen(unsigned int substage) {
    msp_status leastatus;
    unsigned int i;
    fsgsub *s = sub + substage;

    /* Show current status of buffer. */
    P1OUT ^= BIT1;
    i = subtxfm.length;
    while (i--) {
        int y = s->buf[s->cur][i];
        if (y < 0) midisend((_uq8)(((y + 0x80) & 0xFF00) >> 8));
        else midisend((_uq8)((y & 0xFF00) >> 8));
        midiwait();
    }

    /* Pause to allow user to inspect. */
    while (P5IN & BIT6);

    /* Now do the FFT. */
    leastatus = msp_fft_fixed_q15(&subtxfm, s->buf[s->cur]);
    msp_checkStatus(leastatus);

    /* Show current status of spectrum. */
    P1OUT ^= BIT1;
    i = subtxfm.length;
    while (i--) {
        int y = s->buf[s->cur][i];
        if (y < 0) midisend((_uq8)(((y + 0x80) & 0xFF00) >> 8));
        else midisend((_uq8)((y & 0xFF00) >> 8));
        midiwait();
    }

    /* Pause to allow user to inspect. */
    while (P5IN & BIT6);

    /* Write to the substage's spectrum. */
    i = subtxfm.length >> 1;
    while (i--) subspec[substage][i] = abs2(s->buf[s->cur] + (i << 1));

    /* Swap buffers. */
    s->cur ^= 1;
}

/* Updates a substage based on the source buffer. */
static inline void subupdate(unsigned int substage, _q15 *src) {
    msp_status leastatus;
    unsigned int i, exp;
    fsgsub *s = sub + substage;

    /* Send the source buffer the low-pass filter. */
    leastatus = msp_biquad_df1_q15(&s->iir, src, iirout);
    msp_checkStatus(leastatus);

    /* Get the downsample factor's exponent. */
    i = s->iir.length;
    exp = 1;
    while (i != subtxfm.length) {
        i >>= 1;
        exp++;
    }

    /* Now write to the buffer. */
    i = subtxfm.length >> 1;
    while(i--) s->buf[s->cur][s->max + i] = s->buf[s->cur ^ 1u][i] = avg(iirout + (i << exp), exp);
}

/* Regenerates the top stage's magnitude-squared frequency spectrum. */
static inline void topregen(void) {
    unsigned int i;
    msp_status leastatus;

    /* Show current status of buffer. */
    P1OUT ^= BIT1;
    i = top.txfm.length;
    while (i--) {
        int y = top.buf[top.cur ^ 1][i];
        if (y < 0) midisend((_uq8)(((y + 0x80) & 0xFF00) >> 8));
        else midisend((_uq8)((y & 0xFF00) >> 8));
        midiwait();
    }

    /* Pause to allow user to inspect. */
    while (P5IN & BIT6);

    /* Now do the FFT. */
    leastatus = msp_fft_fixed_q15(&top.txfm, top.buf[top.cur ^ 1]);
    msp_checkStatus(leastatus);

    /* Show current status of spectrum. */
    P1OUT ^= BIT1;
    i = top.txfm.length;
    while (i--) {
        int y = top.buf[top.cur ^ 1][i];
        if (y < 0) midisend((_uq8)(((y + 0x80) & 0xFF00) >> 8));
        else midisend((_uq8)((y & 0xFF00) >> 8));
        midiwait();
    }

    /* Pause to allow user to inspect. */
    while (P5IN & BIT6);

    /* Write to the top stage's spectrum. */
    i = top.txfm.length >> 1;
    while (i--) topspec[i] = abs2(top.buf[top.cur ^ 1] + (i << 1));
}

void fsginit(void) {
    /* Initialize stages. Nothing to do for now. */
}

void fsgregen(void) {
    unsigned int i;

    /* Update lower stages first. */
    i = SUBSTAGE_COUNT;
    while (i--) subupdate(i, i == (SUBSTAGE_COUNT - 1) ? top.buf[top.cur ^ 1] : sub[i + 1].buf[sub[i + 1].cur]);

    /* Now do the FFTs. */
    topregen();

    i = SUBSTAGE_COUNT;
    while (i--) subregen(i);
}

inline void fsgupdate(int x) {
    /* Append sample to top sample buffer. */
    top.buf[top.cur][top.i] = x;

    /* Increment sample buffer index. */
    top.i++;

    /* Wake main thread if the buffer is full. */
    if (top.i == TOPSTAGE_SIZE) {
        top.cur ^= 1;
    }
}

void fsgwait(void) {
    while (wait) __low_power_mode_0();
}
