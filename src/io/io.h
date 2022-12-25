#ifndef IO_H
#define IO_H

unsigned char port_io_input_byte();
unsigned short port_io_input_word();

void port_io_out_byte(unsigned short port, unsigned char byte);
void port_io_out_word(unsigned short port, unsigned short word);

void io_test();

#endif