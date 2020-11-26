/*
 * io.h
 *
 * Input/Output Port Driver
 *  Created on: Nov 17, 2020
 *      Author: andob
 */

#ifndef IO_H_
#define IO_H_

typedef enum _ioport {
    IOPORT_1, IOPORT_2, IOPORT_3, IOPORT_4, IOPORT_5,
    IOPORT_6, IOPORT_7, IOPORT_8, IOPORT_J, IOPORT_COUNT
} ioport;

/* Interrupt handler for a port. The return value determines if the edge mode will toggle. */
typedef int (*iofunc)(char in);

/* Enables the I/O ports. */
inline void ioenable(void);

/* Gets the input value of an 8-bit port. */
FASTFUNC char ioget(ioport port, char mask);

/* Sets the interrupt handler for a port and enables interrupts on the corresponding pins.  Mask 1 toggles the edge mode. */
/* The default edge mode is rising edge. */
inline void iohandler(ioport port, char mask0, char mask1, iofunc handler);

/* Sets the direction to input. Mask 1 enables the pull-up/down resistor. */
inline void ioin(ioport port, char mask0, char mask1);

/* Sets I/O ports to default configuration (output 0). */
inline void ioinit(void);

/* Sets the special function for a port's pins. */
inline void iomode(ioport port, char mask0, char mask1);

/* Sets the direction to output. */
inline void ioout(ioport port, char mask);

/* Resets each unmasked bit of an output port, or the default value of an input port. */
FASTFUNC void iorst(ioport port, char mask);

/* Sets each unmasked bit of an output port, or the default value of an input port. */
FASTFUNC void ioset(ioport port, char mask);

/* Toggles each unmasked bit of an output port, or the default value of an input port. */
FASTFUNC void iotog(ioport port, char mask);

#endif /* IO_H_ */
