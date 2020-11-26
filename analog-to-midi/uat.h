/*
 * uat.h
 *
 * UAT Driver
 *  Created on: Nov 18, 2020
 *      Author: andob
 */

/* Notice how the R is missing because we don't receive anything. */

#ifndef UAT_H_
#define UAT_H_

#define UAT_SEL0 0
#define UAT_SEL1 IO_UAT_PIN

/* Enables the UAT. */
inline void uatenable(void);

/* Initializes the UAT driver. */
inline void uatinit(void);

/* Sends a character. */
FASTFUNC int uatoutc(char c);

/* Sends a string. */
FASTFUNC int uatouts(const char *c, unsigned int n);

/* Waits until another character can be sent. */
inline void uatwait(void);

#endif /* UAT_H_ */
