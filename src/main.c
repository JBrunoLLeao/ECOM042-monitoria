#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#include "board_io.h"

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	const int sequence[] = {0, 1, 0, 1};

	if (io_init() < 0) {
		printk("Falha ao inicializar LED/botao\n");
		return -1;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sequence); i++) {
		gpio_emul_input_set(button.port, button.pin, sequence[i]);

		int button_state = button_read();

		led_set(button_state);

		int led_state = gpio_emul_output_get(led.port, led.pin);

		printk("Button: %d -> LED: %d\n", button_state, led_state);
	}

	return 0;
}