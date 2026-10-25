#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "cmd.h"
#include "uart_xport.h"

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(shell_led), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(shell_btn), gpios);

static int led_set(bool on)
{
	return gpio_pin_set_dt(&led, on);
}

static int led_get(void)
{
	return gpio_pin_get_dt(&led);
}

static int btn_get(void)
{
	return gpio_pin_get_dt(&btn);
}

static const struct cmd_hw hw = {
	.led_set = led_set,
	.led_get = led_get,
	.btn_get = btn_get,
};

int main(void)
{
	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&btn)) {
		printk("GPIO nao pronto\n");
		return -1;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&btn, GPIO_INPUT);

	int ret = xport_init(DEVICE_DT_GET(DT_NODELABEL(uart1)), &hw);

	if (ret < 0) {
		printk("xport_init falhou: %d\n", ret);
		return ret;
	}
	printk("Mini-shell pronta na uart1\n");
	return 0;
}
