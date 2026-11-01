#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "ledmux.h"

#define SAMPLE_MS 2

static const struct gpio_dt_spec pins[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};
#define NUM_LEDS ARRAY_SIZE(pins)

/* controlador do led2, que no overlay tem zephyr,deferred-init */
static const struct device *const deferred_ctlr =
	DEVICE_DT_GET(DT_GPIO_CTLR(DT_ALIAS(led2), gpios));

/* lÊ o pino de verdade (no GPIO emulado), não o estado interno do modulo */
static int level(size_t i)
{
	return gpio_emul_output_get(pins[i].port, pins[i].pin);
}

struct obs {
	int changes;
	int last;
	bool saw_on;
	bool saw_off;
};

/* fica olhando os n primeiros LEDs por window_ms, amostrando de 2 em 2 ms.
 * Conta quantas vezes o pino mudou e se chegou a ver ligado e desligado */
static void observe(struct obs *o, size_t n, int window_ms)
{
	for (size_t i = 0; i < n; i++) {
		int v = level(i);

		o[i] = (struct obs){.last = v, .saw_on = (v == 1), .saw_off = (v == 0)};
	}

	for (int t = 0; t < window_ms; t += SAMPLE_MS) {
		k_msleep(SAMPLE_MS);
		for (size_t i = 0; i < n; i++) {
			int v = level(i);

			if (v != o[i].last) {
				o[i].changes++;
				o[i].last = v;
			}
			o[i].saw_on |= (v == 1);
			o[i].saw_off |= (v == 0);
		}
	}
}

/* suite 0: precisa rodar antes da outra, porque aqui o controlador ainda nao
 * está pronto. Confio na ordem alfabetica dos nomes (por isso o 0_ e o 1_) */

ZTEST(ledmux_0_init_fail, test_init_fails_when_controller_not_ready)
{
	zassert_false(device_is_ready(deferred_ctlr),
		      "controlador deveria estar não pronto (suite rodou fora de ordem?)");
	zassert_equal(ledmux_init(), -ENODEV);
	/* sem init válido as outras chamadas tambem tem que falhar */
	zassert_equal(ledmux_on(0), -ENODEV);
}

ZTEST_SUITE(ledmux_0_init_fail, NULL, NULL, NULL, NULL, NULL);

/* suite 1: o resto dos testes */

static void *suite_setup(void)
{
	/* agora sim, inicializa na mão o controlador que estava atrasado */
	int ret = device_init(deferred_ctlr);

	zassert_true(ret == 0 || ret == -EALREADY, "device_init falhou: %d", ret);
	zassert_true(device_is_ready(deferred_ctlr));
	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(ledmux_init());
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);
	ledmux_apply_all(LEDMUX_ACT_OFF);
}

ZTEST(ledmux_1_behavior, test_init_ok_and_all_off)
{
	zassert_equal(ledmux_count(), NUM_LEDS);
	for (size_t i = 0; i < NUM_LEDS; i++) {
		zassert_equal(level(i), 0, "LED %u deveria iniciar desligado", (unsigned int)i);
	}
}

ZTEST(ledmux_1_behavior, test_reinit_does_not_break_timer)
{
	struct obs o[1];

	zassert_ok(ledmux_blink(0, LEDMUX_FAST));
	k_msleep(100);

	/* init de novo tem que parar o pisca e apagar */
	zassert_ok(ledmux_init());
	observe(o, 1, 200);
	zassert_equal(o[0].changes, 0);
	zassert_equal(o[0].last, 0);

	/* e o timer tem que continuar funcionando */
	zassert_ok(ledmux_blink(0, LEDMUX_FAST));
	observe(o, 1, 200);
	zassert_true(o[0].changes >= 2, "pisca não voltou depois do re-init");
}

ZTEST(ledmux_1_behavior, test_on_off_toggle_change_pin)
{
	zassert_ok(ledmux_on(0));
	zassert_equal(level(0), 1);

	zassert_ok(ledmux_off(0));
	zassert_equal(level(0), 0);

	zassert_ok(ledmux_toggle(0));
	zassert_equal(level(0), 1);
	zassert_ok(ledmux_toggle(0));
	zassert_equal(level(0), 0);

	/* os outros nao podem ter mexido */
	zassert_equal(level(1), 0);
	zassert_equal(level(2), 0);
}

ZTEST(ledmux_1_behavior, test_blink_pin_goes_on_and_off)
{
	struct obs o[1];

	zassert_ok(ledmux_blink(0, LEDMUX_NORMAL));
	observe(o, 1, 5 * CONFIG_LEDMUX_NORMAL_MS);
	zassert_true(o[0].saw_on, "nunca ligou");
	zassert_true(o[0].saw_off, "nunca desligou");
	zassert_true(o[0].changes >= 3);
}

ZTEST(ledmux_1_behavior, test_faster_led_toggles_more)
{
	struct obs o[2];

	zassert_ok(ledmux_blink(0, LEDMUX_FAST));
	zassert_ok(ledmux_blink(1, LEDMUX_SLOW));
	observe(o, 2, 1000);

	zassert_true(o[1].changes >= 2, "LED lento deveria trocar algumas vezes");
	zassert_true(o[0].changes > o[1].changes, "rapido=%d lento=%d", o[0].changes, o[1].changes);
}

ZTEST(ledmux_1_behavior, test_stopping_one_keeps_other_blinking)
{
	struct obs o[2];

	zassert_ok(ledmux_blink(0, LEDMUX_FAST));
	zassert_ok(ledmux_blink(1, LEDMUX_FAST));
	k_msleep(100);

	zassert_ok(ledmux_off(1));
	observe(o, 2, 300);
	zassert_true(o[0].changes >= 3, "LED 0 deveria continuar piscando");
	zassert_equal(o[1].changes, 0, "LED 1 deveria ter parado");
	zassert_equal(o[1].last, 0);
}

ZTEST(ledmux_1_behavior, test_on_cancels_blink)
{
	struct obs o[1];

	zassert_ok(ledmux_blink(0, LEDMUX_FAST));
	k_msleep(100);
	zassert_ok(ledmux_on(0));
	observe(o, 1, 200);
	zassert_equal(o[0].changes, 0);
	zassert_equal(o[0].last, 1);
}

ZTEST(ledmux_1_behavior, test_off_cancels_blink)
{
	struct obs o[1];

	zassert_ok(ledmux_blink(0, LEDMUX_FAST));
	k_msleep(100);
	zassert_ok(ledmux_off(0));
	observe(o, 1, 200);
	zassert_equal(o[0].changes, 0);
	zassert_equal(o[0].last, 0);
}

ZTEST(ledmux_1_behavior, test_toggle_cancels_blink)
{
	struct obs o[1];

	zassert_ok(ledmux_blink(0, LEDMUX_FAST));
	k_msleep(100);
	zassert_ok(ledmux_toggle(0));

	int after_toggle = level(0);

	observe(o, 1, 200);
	zassert_equal(o[0].changes, 0);
	zassert_equal(o[0].last, after_toggle);
}

ZTEST(ledmux_1_behavior, test_apply_all)
{
	struct obs o[NUM_LEDS];

	zassert_ok(ledmux_apply_all(LEDMUX_ACT_ON));
	for (size_t i = 0; i < NUM_LEDS; i++) {
		zassert_equal(level(i), 1);
	}

	zassert_ok(ledmux_apply_all(LEDMUX_ACT_OFF));
	for (size_t i = 0; i < NUM_LEDS; i++) {
		zassert_equal(level(i), 0);
	}

	zassert_ok(ledmux_apply_all(LEDMUX_ACT_TOGGLE));
	for (size_t i = 0; i < NUM_LEDS; i++) {
		zassert_equal(level(i), 1);
	}

	zassert_ok(ledmux_apply_all(LEDMUX_ACT_BLINK_FAST));
	observe(o, NUM_LEDS, 300);
	for (size_t i = 0; i < NUM_LEDS; i++) {
		zassert_true(o[i].changes >= 3, "LED %u não piscou", (unsigned int)i);
	}
}

ZTEST(ledmux_1_behavior, test_invalid_args)
{
	size_t bad = ledmux_count();

	zassert_equal(ledmux_on(bad), -EINVAL);
	zassert_equal(ledmux_off(bad), -EINVAL);
	zassert_equal(ledmux_toggle(bad), -EINVAL);
	zassert_equal(ledmux_blink(bad, LEDMUX_SLOW), -EINVAL);
	zassert_equal(ledmux_blink(0, LEDMUX_PATTERN_COUNT), -EINVAL);
	zassert_equal(ledmux_blink(0, (enum ledmux_pattern)99), -EINVAL);
	zassert_equal(ledmux_apply(0, LEDMUX_ACT_COUNT), -EINVAL);
	zassert_equal(ledmux_get(bad), -EINVAL);

	/* e o LED válido não foi afetado */
	zassert_equal(level(0), 0);
}

ZTEST_SUITE(ledmux_1_behavior, NULL, suite_setup, before_each, after_each, NULL);
