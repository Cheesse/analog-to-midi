/*
 * config.h
 *
 * Master Configuration Header
 *  Created on: Nov 17, 2020
 *      Author: andob
 */

/* NOTE: If you change anything here, you will probably have to do a full rebuild. */

#ifndef CONFIG_H_
#define CONFIG_H_

/* ADC */

/* To change the ADC channel, you must change the port and pin, and vice versa. Refer to the datasheet. */
#define ADC_CHANNEL 8

/* Enable the ADC voltage reference output. */
#define ADC_REF_ENABLE 1

/* The ADC's sample rate (max 50 kHz). The actual sample rate may be slightly different. Must change adc.c with any change. */
#define ADC_SAMPLE_RATE 12800

/* CLOCK */

/* Whether to use the DCOSC or MODOSC oscillators. */
#define SMCLK_USE_DCOSC 1

/* Subsystem master clock division exponent (max 5). The frequency of the subsystem master clock is 5 MHz >> SMCLK_DIV_EXP. */
#define SMCLK_DIV_EXP 2

/* FSG */

#define FSG_TOPSTAGE_SIZE_EXP 9
#define FSG_TOPSTAGE_SIZE (1 << 9)

/* FSG_SUBSTAGE_SIZE must be greater than 2 ^ FSG_SUBSTAGE_COUNT! */
#define FSG_SUBSTAGE_COUNT 2
#define FSG_SUBSTAGE_SIZE 64

/* IO */

/* This pin will receive an analog signal and pass it to the ADC. */
#define IO_ADC_PORT IOPORT_4
#define IO_ADC_PIN PIN0

/* This pin will send an SPI clock signal to the USB controller. */
#define IO_SPI_CLK_PORT
#define IO_SPI_CLK_PIN

/* This pin will receive data from the USB controller. */
#define IO_SPI_MISO_PORT
#define IO_SPI_MISO_PIN

/* This pin will send MIDI data to the USB controller. */
#define IO_SPI_MOSI_PORT
#define IO_SPI_MOSI_PIN

/* This pin will receive the POWERSRC signal from the power multiplexer. */
#define IO_SIG_POWERSRC_PORT
#define IO_SIG_POWERSRC_PIN

/* This pin will receive the USBCONN signal from the USB controller. */
#define IO_SIG_USBCONN_PORT
#define IO_SIG_USBCONN_PIN

/* This pin will receive the USBSUSP signal from the USB controller. */
#define IO_SIG_USBSUSP_PORT
#define IO_SIG_USBSUSP_PIN

/* This pin will output data from the UAT. */
#define IO_UAT_PORT IOPORT_2
#define IO_UAT_PIN PIN5

/* This pin will output the inverse of the UAT output. The port and pin should correspond to the UCA1 peripheral. */
#define IO_UAT_INVERT_PORT IOPORT_2
#define IO_UAT_INVERT_PIN PIN4

/* This pin will take the UAT output as input. */
#define IO_UAT_LOOPBACK_PORT IOPORT_2
#define IO_UAT_LOOPBACK_PIN PIN6

/* MIDI */

/* Amount of polyphony in the MIDI stream generation. */
#define MIDI_POLYPHONY 6

/* UAT */

/* Baud rate of the UAT. Valid values are only 31250. */
#define UAT_BAUD_RATE 31250
//#define UAT_BAUD_RATE 32258

/* Special modulation pattern for the baud rate generator. Refer to the datasheet to determine the correct value. */
//#define UAT_BAUD_MOD 0x52
#define UAT_BAUD_MOD 0x00

/* SPI */

/* DEBUG */

/* Toggle an output pin based on the sample rate of the ADC. Useful for measuring the ADC sample rate. */
#define DEBUG_ADC_PERIOD 0
#define DEBUG_ADC_PERIOD_PORT IOPORT_1
#define DEBUG_ADC_PERIOD_PIN PIN2

/* Toggle an output pin on entering and exiting the FSG. Useful for benchmarking the FSG. */
#define DEBUG_FSG_PERIOD 0
#define DEBUG_FSG_PERIOD_PORT IOPORT_1
#define DEBUG_FSG_PERIOD_PIN PIN2

/* Print data from the FSG. Set to 1 to print sample buffers and wait. Set to 2 to print frequency spectra. */
#define DEBUG_FSG_PRINT 0

/* This pin is used as input to determine if the code should continue. Used with DEBUG_FSG_PRINT = 1. */
#define DEBUG_FSG_CONT_PORT IOPORT_5
#define DEBUG_FSG_CONT_PIN PIN6

/* Toggle an output pin based on the iteration rate of the whole system. Useful for measuring the iteration period. */
#define DEBUG_ITER_PERIOD 0
#define DEBUG_ITER_PERIOD_PORT IOPORT_1
#define DEBUG_ITER_PERIOD_PIN PIN4

/* Repeatedly output a C4 note and rest to test MIDI connectivity. Turn on a pin when the device is transmitting. */
#define DEBUG_MIDI 0
#define DEBUG_MIDI_PORT IOPORT_1
#define DEBUG_MIDI_PIN PIN5

/* Baud rate of the debug UART. Valid values are 9600 and 115200. Used to transfer data between the device and a computer. */
/* The port and pin should correspond to UCA0, since this peripheral is connected to a UART-USB backchannel on the dev board.
 * If this peripheral is not connected to the backchannel, then it must be changed in the code. */
#define DEBUG_UART_BAUD_RATE 115200
#define DEBUG_UART_PORT IOPORT_2
#define DEBUG_UART_PIN PIN0

#endif /* CONFIG_H_ */
