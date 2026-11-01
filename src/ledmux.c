#include "ledmux.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ledmux, CONFIG_LOG_DEFAULT_LEVEL);

/* estado de cada LED */

struct led {
	struct gpio_dt_spec spec; /* pino, vem do devicetree */
	enum ledmux_pattern pattern;
	uint32_t period_ms; /* tempo entre uma troca e outra */
	int64_t deadline;   /* hora da proxima troca (uptime em ms) */
	bool blinking;
	bool level; /* ligado ou desligado agora */
};

/* entra no array um LED pra cada alias led0..led7 que existir.
 * Quer mais um LED? Cria o alias no overlay, nao precisa mexer aqui */
#define LED_ENTRY(n)                                                                               \
	COND_CODE_1(DT_NODE_EXISTS(DT_ALIAS(led##n)),                                              \
		    ({.spec = GPIO_DT_SPEC_GET(DT_ALIAS(led##n), gpios)}, ), ())

static struct led leds[] = {LED_ENTRY(0) LED_ENTRY(1) LED_ENTRY(2) LED_ENTRY(3) LED_ENTRY(4)
				    LED_ENTRY(5) LED_ENTRY(6) LED_ENTRY(7)};

BUILD_ASSERT(ARRAY_SIZE(leds) >= 1, "Nenhum alias ledN encontrado no devicetree");
BUILD_ASSERT(ARRAY_SIZE(leds) <= LEDMUX_MAX_LEDS, "LEDs demais");
BUILD_ASSERT(LEDMUX_ACT_BLINK_FAST - LEDMUX_ACT_BLINK_SLOW == LEDMUX_PATTERN_COUNT - 1,
	     "Ações BLINK_* fora de ordem em relação aos padrões");

/* periodos vem do Kconfig, nada de número solto aqui */
static const uint32_t pattern_ms[LEDMUX_PATTERN_COUNT] = {
	[LEDMUX_SLOW] = CONFIG_LEDMUX_SLOW_MS,
	[LEDMUX_NORMAL] = CONFIG_LEDMUX_NORMAL_MS,
	[LEDMUX_FAST] = CONFIG_LEDMUX_FAST_MS,
};

/* o que a thread e o callback do timer dividem */

static struct k_spinlock lock; /* protege leds[] e initialized, porque thread e ISR mexem neles */
static bool initialized;

static void expiry(struct k_timer *timer);

/* o unico timer do projeto. One-shot, entao eu rearmo toda vez que ele dispara */
K_TIMER_DEFINE(blink_timer, expiry, NULL);

/* funções internas, todas chamadas com o lock já pego */

static void hw_write(struct led *l, bool on)
{
	l->level = on;
	(void)gpio_pin_set_dt(&l->spec, on);
}

/* programa o timer pro deadline mais próximo entre quem esta piscando.
 * Se ninguem pisca, para o timer */
static void reschedule(int64_t now)
{
	int64_t next = INT64_MAX;

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		if (leds[i].blinking) {
			next = MIN(next, leds[i].deadline);
		}
	}

	if (next == INT64_MAX) {
		k_timer_stop(&blink_timer);
		return;
	}

	k_timer_start(&blink_timer, K_MSEC(MAX(next - now, (int64_t)1)), K_NO_WAIT);
}

/* comandos: cada ação é uma função, e a tabela mais abaixo liga o enum a elas */

typedef void (*cmd_fn)(struct led *l, uint32_t arg, int64_t now);

struct cmd {
	cmd_fn fn;
	uint32_t arg;
};

static void cmd_off(struct led *l, uint32_t arg, int64_t now)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(now);
	l->blinking = false;
	hw_write(l, false);
}

static void cmd_on(struct led *l, uint32_t arg, int64_t now)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(now);
	l->blinking = false;
	hw_write(l, true);
}

static void cmd_toggle(struct led *l, uint32_t arg, int64_t now)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(now);
	l->blinking = false;
	hw_write(l, !l->level);
}

/* aqui o arg e o índice do padrao */
static void cmd_blink(struct led *l, uint32_t arg, int64_t now)
{
	l->pattern = (enum ledmux_pattern)arg;
	l->period_ms = pattern_ms[arg];
	l->deadline = now + l->period_ms;
	l->blinking = true;
	hw_write(l, true);
}

static const struct cmd cmds[LEDMUX_ACT_COUNT] = {
	[LEDMUX_ACT_OFF] = {cmd_off, 0},
	[LEDMUX_ACT_ON] = {cmd_on, 0},
	[LEDMUX_ACT_TOGGLE] = {cmd_toggle, 0},
	[LEDMUX_ACT_BLINK_SLOW] = {cmd_blink, LEDMUX_SLOW},
	[LEDMUX_ACT_BLINK_NORMAL] = {cmd_blink, LEDMUX_NORMAL},
	[LEDMUX_ACT_BLINK_FAST] = {cmd_blink, LEDMUX_FAST},
};

/* callback do timer. Roda em ISR, entao tem que ser curto e sem log/printk */

static void expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_spinlock_key_t key = k_spin_lock(&lock);
	int64_t now = k_uptime_get();

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		struct led *l = &leds[i];

		if (l->blinking && now >= l->deadline) {
			hw_write(l, !l->level);
			/* soma o período em cima do deadline antigo (e nao em cima do "agora")
			 * pra fase nao ir derivando. O while é só pro caso de atrasar muito */
			do {
				l->deadline += l->period_ms;
			} while (l->deadline <= now);
		}
	}

	reschedule(now);
	k_spin_unlock(&lock, key);
}

/* "um ou todos" fica só aqui: transforma o índice num intervalo [first, end) */

static int resolve_range(size_t led, size_t *first, size_t *end)
{
	*first = 0;
	*end = ARRAY_SIZE(leds);

	if (led == LEDMUX_ALL) {
		return 0;
	}

	if (led >= ARRAY_SIZE(leds)) {
		LOG_ERR("invalid LED index %u (max %u)", (unsigned int)led,
			(unsigned int)(ARRAY_SIZE(leds) - 1));
		return -EINVAL;
	}

	*first = led;
	*end = led + 1;
	return 0;
}

/* API publica */

size_t ledmux_count(void)
{
	return ARRAY_SIZE(leds);
}

int ledmux_init(void)
{
	/* primeiro vejo se todos os controladores estao prontos, antes de mexer em qualquer coisa
	 */
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		if (!gpio_is_ready_dt(&leds[i].spec)) {
			LOG_ERR("LED %u: GPIO não preparado!", (unsigned int)i);
			return -ENODEV;
		}
	}

	/* para o timer e zera tudo. Da pra chamar o init varias vezes porque o timer
	 * é estatico, eu só paro ele, nunca recrio */
	k_spinlock_key_t key = k_spin_lock(&lock);

	initialized = false;
	k_timer_stop(&blink_timer);
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		leds[i].blinking = false;
		leds[i].level = false;
		leds[i].pattern = LEDMUX_SLOW;
		leds[i].period_ms = 0;
		leds[i].deadline = 0;
	}
	k_spin_unlock(&lock, key);

	/* agora sim configura os pinos como saída, apagados */
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		int ret = gpio_pin_configure_dt(&leds[i].spec, GPIO_OUTPUT_INACTIVE);

		if (ret < 0) {
			LOG_ERR("LED %u: configure failed (%d)", (unsigned int)i, ret);
			return ret;
		}
	}

	key = k_spin_lock(&lock);
	initialized = true;
	k_spin_unlock(&lock, key);
	return 0;
}

int ledmux_apply(size_t led, enum ledmux_action action)
{
	size_t first, end;
	int ret = resolve_range(led, &first, &end);

	if (ret < 0) {
		return ret;
	}

	if ((unsigned int)action >= LEDMUX_ACT_COUNT) {
		LOG_ERR("Ação inválida %d", (int)action);
		return -EINVAL;
	}

	const struct cmd *cmd = &cmds[action];
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (!initialized) {
		k_spin_unlock(&lock, key);
		LOG_ERR("not initialized");
		return -ENODEV;
	}

	int64_t now = k_uptime_get();

	for (size_t i = first; i < end; i++) {
		cmd->fn(&leds[i], cmd->arg, now);
	}

	reschedule(now);
	k_spin_unlock(&lock, key);
	return 0;
}

int ledmux_apply_all(enum ledmux_action action)
{
	return ledmux_apply(LEDMUX_ALL, action);
}

int ledmux_on(size_t led)
{
	return ledmux_apply(led, LEDMUX_ACT_ON);
}

int ledmux_off(size_t led)
{
	return ledmux_apply(led, LEDMUX_ACT_OFF);
}

int ledmux_toggle(size_t led)
{
	return ledmux_apply(led, LEDMUX_ACT_TOGGLE);
}

int ledmux_blink(size_t led, enum ledmux_pattern pattern)
{
	if ((unsigned int)pattern >= LEDMUX_PATTERN_COUNT) {
		LOG_ERR("invalid pattern %d", (int)pattern);
		return -EINVAL;
	}

	return ledmux_apply(led, (enum ledmux_action)(LEDMUX_ACT_BLINK_SLOW + pattern));
}

int ledmux_get(size_t led)
{
	if (led >= ARRAY_SIZE(leds)) {
		LOG_ERR("invalid LED index %u", (unsigned int)led);
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	int level = leds[led].level;

	k_spin_unlock(&lock, key);
	return level;
}
