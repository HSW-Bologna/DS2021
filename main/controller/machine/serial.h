#ifndef SERIAL_H_INCLUDED
#define SERIAL_H_INCLUDED


#include <termios.h>


int serial_set_interface_attribs(int fd, int speed);
void serial_set_timeout(int fd, int mcount, int decsec);
void serial_set_mincount(int fd, int mcount);
int serial_open_tty(char *portname);


#endif