/*
 * defs.h
 *
 * Defines
 *  Created on: Nov 17, 2020
 *      Author: andob
 */

#include <stdint.h>

#ifndef DEFS_H_
#define DEFS_H_

#define RAMFUNC __attribute((ramfunc))
#define FASTFUNC inline RAMFUNC
//#define RAMFUNC
//#define FASTFUNC

#define PIN0 0b00000001
#define PIN1 0b00000010
#define PIN2 0b00000100
#define PIN3 0b00001000
#define PIN4 0b00010000
#define PIN5 0b00100000
#define PIN6 0b01000000
#define PIN7 0b10000000

#endif /* DEFS_H_ */
