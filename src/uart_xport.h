#ifndef UART_XPORT_H
#define UART_XPORT_H

#include <zephyr/device.h>
#include "cmd.h"

int xport_init(const struct device *uart, const struct cmd_hw *hw);

#endif
