/*
 * duart.h
 *
 * Debug UART Driver
 *  Created on: Nov 18, 2020
 *      Author: andob
 */

#ifndef DUART_H_
#define DUART_H_

#define UART_SEL0 0
#define UART_SEL1 DEBUG_UART_PIN

/* Enables the debug UART. */
inline void duartenable(void);

/* Initializes the debug UART driver. */
inline void duartinit(void);

/* Sends a character. Returns 1 if the write buffer is full. */
FASTFUNC int duartoutc(char c);

/* Sends a string. Returns 1 if the write buffer cannot hold the string. */
FASTFUNC int duartouts(const char *s, unsigned int n);

/* Waits until another character can be sent. */
inline void duartwait(void);

#endif /* DUART_H_ */
