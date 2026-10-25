#include "line.h"
#include "uart_xport.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#define RESP_MAX 128

RING_BUF_DECLARE(rx_rb, 128);
static K_SEM_DEFINE(rx_sem, 0, 1);
static atomic_t rx_stalled;

static const struct device *uart_dev;
static const struct cmd_hw *hw_ops;

K_THREAD_STACK_DEFINE(xport_stack, 1536);
static struct k_thread xport_thread;

/* ISR: so enfileira. Nunca le mais do que cabe na fila. */
static void uart_isr(const struct device *dev, void *user_data)
{
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (!uart_irq_rx_ready(dev)) {
			break;
		}
		uint8_t *p;
		uint32_t n = ring_buf_put_claim(&rx_rb, &p, ring_buf_space_get(&rx_rb));

		if (n == 0) {
			/* fila cheia: os bytes ficam no FIFO da UART */
			atomic_set(&rx_stalled, 1);
			uart_irq_rx_disable(dev);
			break;
		}
		int r = uart_fifo_read(dev, p, n);

		ring_buf_put_finish(&rx_rb, r > 0 ? r : 0);
		if (r <= 0) {
			break;
		}
		k_sem_give(&rx_sem);
	}
}

static void tx_str(const char *s)
{
	while (*s) {
		uart_poll_out(uart_dev, *s++);
	}
}

static void xport_thread_fn(void *a, void *b, void *c)
{
	struct line line;
	char resp[RESP_MAX];
	uint8_t byte;

	line_init(&line);
	tx_str("> ");

	for (;;) {
		k_sem_take(&rx_sem, K_FOREVER);

		while (ring_buf_get(&rx_rb, &byte, 1) == 1) {
			/* abriu espaco: religa RX se estava parado */
			if (atomic_cas(&rx_stalled, 1, 0)) {
				uart_irq_rx_enable(uart_dev);
			}

			switch (line_feed(&line, byte)) {
			case LINE_CHAR:
				uart_poll_out(uart_dev, byte);
				break;
			case LINE_ERASE:
				tx_str("\b \b");
				break;
			case LINE_DONE:
				tx_str("\r\n");
				cmd_execute(line.buf, hw_ops, resp, sizeof(resp));
				if (resp[0]) {
					tx_str(resp);
					tx_str("\r\n");
				}
				tx_str("> ");
				break;
			case LINE_TOO_LONG:
				tx_str("\r\nERR: line too long\r\n> ");
				break;
			default:
				break;
			}
		}
		/* garante religar mesmo se a fila esvaziou */
		if (atomic_cas(&rx_stalled, 1, 0)) {
			uart_irq_rx_enable(uart_dev);
		}
	}
}

int xport_init(const struct device *uart, const struct cmd_hw *hw)
{
	if (uart == NULL || !device_is_ready(uart)) {
		return -ENODEV;
	}
	uart_dev = uart;
	hw_ops = hw;

	int ret = uart_irq_callback_user_data_set(uart, uart_isr, NULL);

	if (ret < 0) {
		return ret;
	}
	k_thread_create(&xport_thread, xport_stack, K_THREAD_STACK_SIZEOF(xport_stack),
			xport_thread_fn, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	uart_irq_rx_enable(uart);
	return 0;
}
