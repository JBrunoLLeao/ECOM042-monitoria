#ifndef IO_H_
#define IO_H_

int io_init(void);

void io_button_simulate(int pressed);

int io_button_read(void);

void io_led_write(int value);

int io_led_read(void);

#endif
