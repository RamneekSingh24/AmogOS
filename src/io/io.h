#ifndef IO_H
#define IO_H


unsigned char in_byte(unsigned short port);
unsigned short in_word(unsigned short port);

void out_byte(unsigned short port, unsigned char val);
void out_word(unsigned short port, unsigned short val);

#endif