/*
 * cpu.h
 *
 * CPU Driver
 *  Created on: Nov 17, 2020
 *      Author: andob
 */

#ifndef CPU_H_
#define CPU_H_

/* Frequency of the Master clock. Fixed at 16 MHz. */
#define MCLK_FREQ 16000000

/* Module oscillator typical frequency. */
#define MODOSC_FREQ 4800000

/* Subsystem master clock. */
#if SMCLK_USE_DCOSC
#define SMCLK_FREQ (MCLK_FREQ >> SMCLK_DIV_EXP)
#else
#define SMCLK_FREQ (MODOSC_FREQ >> SMCLK_DIV_EXP)
#endif /* SMCLK_USE_DCOSC */

/* Sets up the CPU and the clocks to a hard-coded configuration. The CPU will use the highest clock rate possible. */
inline void cpuinit(void);

/* Sleeps until an interrupt sets the value at the address given to 0. If NULL is passed then it will not check any address. */
inline void cpuwait(volatile int *ptr);

#endif /* CPU_H_ */
