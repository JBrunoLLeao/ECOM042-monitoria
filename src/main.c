#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ledmux.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define MONITOR_STEP_MS 10

int main(void)
{
	int ret = ledmux_init();

	if (ret < 0) {
		LOG_ERR("ledmux_init failed: %d", ret);
		return ret;
	}

	size_t n = ledmux_count();
	int last[LEDMUX_MAX_LEDS] = {0};

	printk("ledmux: %u LEDs\n", (unsigned int)n);

	/* LED 0 lento, LED 1 normal, LED 2 rapido e assim por diante */
	for (size_t i = 0; i < n; i++) {
		ledmux_blink(i, (enum ledmux_pattern)(i % LEDMUX_PATTERN_COUNT));
	}

	for (int64_t t = 0;; t += MONITOR_STEP_MS) {
		/* "monitor" bem simples: só imprime quando algum LED muda de nivel.
		 * Serve pra ver o que tá rolando no native_sim, que nãp tem LED de verdade */
		for (size_t i = 0; i < n; i++) {
			int v = ledmux_get(i);

			if (v != last[i]) {
				printk("[%6lld ms] LED%u -> %s\n", k_uptime_get(), (unsigned int)i,
				       v ? "ON" : "OFF");
				last[i] = v;
			}
		}

		if (t == 8000) {
			printk("desligando so o LED1: os outros mantém a fase\n");
			ledmux_off(1);
		}
		if (t == 14000) {
			printk("todos piscando rápido\n");
			ledmux_apply_all(LEDMUX_ACT_BLINK_FAST);
		}
		if (t == 18000) {
			printk("todos desligados\n");
			ledmux_apply_all(LEDMUX_ACT_OFF);
		}
		/* k_msleep aqui é só do monitor, não faz parte do módulo, então não viola a regra.
		 */
		k_msleep(MONITOR_STEP_MS);
	}

	return 0;
}
