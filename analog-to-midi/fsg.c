/*
 * fsg.c
 *
 * Frequency Spectrum Generation
 *  Created on: Aug 14, 2020
 *      Author: Andrew Obeso
 */

//#include "adc.h"
#include "config.h"
#include "defs.h"

#include "cpu.h"
#include "fsg.h"

#define GLOBAL_Q 15
#define GLOBAL_IQ 30
#include "IQmathLib.h"
#include "QmathLib.h"
#include "DSPLib.h"

/*
#include <stdint.h>
#include <stdbool.h>

#include "DSPLib_types.h"
#include "DSPLib_support.h"
#include "DSPLib_filter.h"
#include "DSPLib_transform.h"
*/

/* LEA MEMORY */

/* Sample buffers. */
DSPLIB_DATA(iirout, FSG_TOPSTAGE_SIZE * 2)
static _q15 iirout[FSG_TOPSTAGE_SIZE];

DSPLIB_DATA(topbuf0, MSP_ALIGN_FFT_Q15(FSG_TOPSTAGE_SIZE))
static _q15 topbuf0[FSG_TOPSTAGE_SIZE];
DSPLIB_DATA(topbuf1, MSP_ALIGN_FFT_Q15(FSG_TOPSTAGE_SIZE))
static _q15 topbuf1[FSG_TOPSTAGE_SIZE];

DSPLIB_DATA(sub1buf, MSP_ALIGN_FFT_Q15(FSG_SUBSTAGE_SIZE))
static _q15 sub1buf[FSG_SUBSTAGE_SIZE];

DSPLIB_DATA(sub0buf, MSP_ALIGN_FFT_Q15(FSG_SUBSTAGE_SIZE))
static _q15 sub0buf[FSG_SUBSTAGE_SIZE];

/* IIR filter coefficients. */
DSPLIB_DATA(topfc, 4)
static const msp_biquad_df1_q15_coeffs topfc = {
    .b0 = _Q15(0.098806456449613),
    .b1By2 = _Q15(-0.188330247089721/2),
    .b2 = _Q15(0.098806456449613),
    .a1By2 = _Q15(1.921409969623980/2),
    .a2 = _Q15(-0.937917143103297)
};

DSPLIB_DATA(subfc, 4)
static const msp_biquad_df1_q15_coeffs subfc = {
    .b0 = _Q15(0.176395745023128),
    .b1By2 = _Q15(0.228176245840090/2),
    .b2 = _Q15(0.176395745023128),
    .a1By2 = _Q15(0.697776133688823/2),
    .a2 = _Q15(-0.518414872842765)
};

/* IIR filter states. */
DSPLIB_DATA(topfs, 4)
static msp_biquad_df1_q15_states topfs;

DSPLIB_DATA(subfs, 4)
static msp_biquad_df1_q15_states subfs;

/* Multiply-Accumulate output. */
DSPLIB_DATA(macout, 4);
static _iq31 macout;

/* NORMAL MEMORY */

/* Gets the frame size of a substage. */
#define subframe(substage) (FSG_SUBSTAGE_SIZE >> (FSG_SUBSTAGE_COUNT - (substage)))

/* IIR info. */
static const msp_biquad_df1_q15_params subiir[FSG_SUBSTAGE_COUNT] = {
    { subframe(1), &subfc, &subfs },
    { FSG_TOPSTAGE_SIZE, &topfc, &topfs }
};

/* Substages. */
static _q15 subbackbuf[FSG_SUBSTAGE_COUNT][FSG_SUBSTAGE_SIZE];
//static const unsigned int submax[FSG_SUBSTAGE_COUNT] = { FSG_SUBSTAGE0_FRAME, FSG_SUBSTAGE1_FRAME };
static _q15 *const subfrontbuf[FSG_SUBSTAGE_COUNT] = { sub0buf, sub1buf };

/* Top stage. */
static _q15 *const topbuf[2] = { topbuf0, topbuf1 };
static volatile unsigned int topcur = 0;
static volatile int wait;

unsigned int framerms;

/* Frequency spectra. */
unsigned char subspec[FSG_SUBSTAGE_COUNT][FSG_SUBSTAGE_SIZE / 2];
unsigned char topspec[FSG_TOPSTAGE_SIZE / 2];

/* Calculate the average of several numbers. */
FASTFUNC static _q15 avg(const _q15 *a, unsigned int count, unsigned int exponent) {
    _iq31 sum;

    sum = 0;
    while (count--) sum += (_iq31)a[count];

    return (_q15)(sum >> exponent);
}

/* Clamped Left Shift. */
FASTFUNC static _q15 clsh(_q15 a, unsigned int shift) {
    _q15 b;

    b = a & 0x8000;
    a <<= shift;

    if ((a & 0x8000) ^ b) a = b ? 0x8000 : 0x7FFF;

    return a;
}

/* Calculate the RMS of a 512-sample buffer. The input must be in LEA memory (for now).*/
static FASTFUNC _uq15 rms(const _q15 *in) {
    static const msp_shift_q15_params shift = { FSG_TOPSTAGE_SIZE, -FSG_TOPSTAGE_SIZE_EXP };
    static const msp_mac_q15_params mac = { FSG_TOPSTAGE_SIZE };
    msp_status leastatus;

    leastatus = msp_shift_q15(&shift, in, iirout);
    msp_checkStatus(leastatus);

    leastatus = msp_mac_q15(&mac, iirout, iirout, &macout);
    msp_checkStatus(leastatus);

    return _IQsqrt(macout >> 1) >> (16 - 1 - 1);
}

/* Regenerates a substage's magnitude-squared frequency spectrum. */
static FASTFUNC void subregen(unsigned int substage) {
    static const msp_fft_q15_params fft = { FSG_SUBSTAGE_SIZE, true };
    msp_status leastatus;
    unsigned int i, j;
    _q15 a, *b;
    unsigned char *s;

    b = subfrontbuf[substage];

    /* Now do the FFT. */
    leastatus = msp_fft_fixed_q15(&fft, b);
    msp_checkStatus(leastatus);

    /* Boost values. */
    i = FSG_SUBSTAGE_SIZE;
    while (i--) b[i] = clsh(b[i], 1);

    /* Write to the substage's spectrum. */
    s = subspec[substage];
    i = FSG_SUBSTAGE_SIZE / 2;
    j = FSG_SUBSTAGE_SIZE;
    while (i--) {
        j -= 2;
        a = _Qmag(b[j], b[j + 1]);
        s[i] = (unsigned char)(a >> (8 - 1));
    }
}

/* Updates a substage based on the source buffer. */
static FASTFUNC void subupdate(unsigned int substage, const _q15 *src) {
    msp_status leastatus;
    static const unsigned int subdownsample[FSG_SUBSTAGE_COUNT] = {
        subframe(1) / subframe(0),
        FSG_TOPSTAGE_SIZE / subframe(1)
    };
    static const unsigned int subdownsampleexp[FSG_SUBSTAGE_COUNT] = { 1, 4 };
    unsigned int a, d, e, i, j, m;
    _q15 *b, *f;
    const msp_biquad_df1_q15_params *iir;

    iir = subiir + substage;
    b = subbackbuf[substage];
    f = subfrontbuf[substage];
    j = iir->length;
    m = subframe(substage);
    e = subdownsampleexp[substage];
    d = FSG_SUBSTAGE_SIZE - m;
    a = subdownsample[substage];

    /* Send the source buffer the low-pass filter. */
    leastatus = msp_biquad_df1_q15(iir, src, iirout);
    msp_checkStatus(leastatus);

    /* Shift the contents of the back buffer. */
    for (i = 0; i != d; i++) b[i] = b[i + m];

    /* Copy the IIR output at specific intervals. */
    i = m;
    while(i--) {
        j -= a;
        //b[d + i] = iirout[j];
        b[d + i] = avg(iirout + j, a, e);
    }

    /* Copy the data to the front buffer with boost. */
    i = FSG_SUBSTAGE_SIZE;
    while (i--) f[i] = clsh(b[i], (FSG_SUBSTAGE_COUNT - 1) - substage);
}

/* Regenerates the top stage's magnitude-squared frequency spectrum. */
static FASTFUNC void topregen(void) {
    static const msp_fft_q15_params fft = { FSG_TOPSTAGE_SIZE, true };
    msp_status leastatus;
    unsigned int i, j;
    _q15 a;
    _q15 *b;

    b = topbuf[topcur ^ 1];

    /* Now do the FFT. */
    leastatus = msp_fft_fixed_q15(&fft, b);
    msp_checkStatus(leastatus);

    /* Boost values. */
    i = FSG_SUBSTAGE_SIZE;
    while (i--) b[i] = clsh(b[i], 1);

    /* Write to the top stage's spectrum. */
    i = FSG_TOPSTAGE_SIZE / 2;
    j = FSG_TOPSTAGE_SIZE;
    while (i--) {
        j -= 2;
        a = _Qmag(b[j], b[j + 1]);
        topspec[i] = (unsigned char)(a >> (8 - 1));
    }
}

#if DEBUG_FSG_PRINT == 1
unsigned char* fsggetsub(unsigned int substage) { return subfrontbuf[substage]; }

unsigned char* fsggettop(void) { return topbuf[topcur ^ 1]; }
#endif /* DEBUG_FSG_PRINT == 1 */

void fsgprep(void) {
    _q15 *b;
    unsigned int i;

    /* Shift, boost, and clamp all samples. */
    b = topbuf[topcur ^ 1];
    i = FSG_TOPSTAGE_SIZE;
    while (i--) b[i] = clsh(b[i] + 0x2800, 2);

    /* Set frame's RMS value. */
    b = topbuf[topcur ^ 1];
    framerms = rms(b);

    /* Update lower stages. */
    if (FSG_SUBSTAGE_COUNT) {
        i = FSG_SUBSTAGE_COUNT - 1;
        subupdate(i, b);
        while (i--) subupdate(i, subfrontbuf[i + 1] + FSG_SUBSTAGE_SIZE - subframe(i + 1));
    }
}

void fsgregen(void) {
    unsigned int i;

    /* Now do the FFTs. */
    topregen();

    i = FSG_SUBSTAGE_COUNT;
    while (i--) subregen(i);
}

int fsgsync(int x) {
    static unsigned int n = 0;

    topbuf[topcur][n] = x;

    /* Increment sample buffer index. */
    n = (n + 1) & (FSG_TOPSTAGE_SIZE - 1);

    /* Update the entire FSG if the buffer is full. */
    if (!n) {
        topcur ^= 1;
        wait = 0;
        return 1;
    }

    return 0;
}

void fsgwait(void) {
    wait = 1;
    cpuwait(&wait);
}
