#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "io.h"

int main(void)
{
	const int sequence[] = {0, 1, 0, 1};

	if (io_init() < 0) {
		printk("Falha ao inicializar LED/botao\n");
		return -1;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sequence); i++) {
		io_button_simulate(sequence[i]);

		int button_state = io_button_read();

		io_led_write(button_state);

		int led_state = io_led_read();

		printk("Button: %d -> LED: %d\n", button_state, led_state);
	}

	return 0;
}
