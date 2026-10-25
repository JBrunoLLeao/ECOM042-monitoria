#ifndef UART_XPORT_H
#define UART_XPORT_H

#include "cmd.h"
#include <zephyr/device.h>

int xport_init(const struct device *uart, const struct cmd_hw *hw);

#endif
