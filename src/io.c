#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>

#include "io.h"

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

int io_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&button)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int io_led_read(void)
{
	return gpio_emul_output_get(led.port, led.pin);
}

void io_button_simulate(int pressed)
{
	gpio_emul_input_set(button.port, button.pin, pressed);
}

int io_button_read(void)
{
	return gpio_pin_get_dt(&button);
}

void io_led_write(int value)
{
	gpio_pin_set_dt(&led, value);
}
