/*************************************************************
**	serial_out.h
**
**	The purpose of this library is to encapsulate the
**	Serial output device, and provide a wite function.
**
**	NOTE: This library is currently pointless, as one
**	could simply call the serial functions themselves,
**	but this library may become more useful for keeping
**	track of what has been sent, or sanitizing the data.
**
**************************************************************/

#ifndef _serial_out_h_
#define _serial_out_h_

#define BAUD 19200

#include <wiringPi.h>
#include <errno.h>

typedef serial_out_t {
	int fd;
	unsigned char data; //copy of last sent data -- maybe useful
} serial_out_t;

int serial_out_init(serial_out_t *serial, char * port); // opens the serial port and sets it to serial->fd
void write_byte(serial_out_t *serial, unsigned char byte); // writes byte to serial port

#endif /*_serial_out_h_*/

