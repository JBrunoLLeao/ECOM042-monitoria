#ifndef LEDMUX_H
#define LEDMUX_H

#include <stddef.h>

/* limite de LEDs (aliases led0 ate led7) */
#define LEDMUX_MAX_LEDS 8

/* passa isso no lugar do indice pra mexer em todos de uma vez */
#define LEDMUX_ALL ((size_t)-1)

enum ledmux_pattern {
	LEDMUX_SLOW,
	LEDMUX_NORMAL,
	LEDMUX_FAST,
	LEDMUX_PATTERN_COUNT,
};

/* cuidado aqui em: os BLINK_* tem que seguir a mesma ordem do enum de cima,
 * o ledmux_blink depende disso */
enum ledmux_action {
	LEDMUX_ACT_OFF,
	LEDMUX_ACT_ON,
	LEDMUX_ACT_TOGGLE,
	LEDMUX_ACT_BLINK_SLOW,
	LEDMUX_ACT_BLINK_NORMAL,
	LEDMUX_ACT_BLINK_FAST,
	LEDMUX_ACT_COUNT,
};

/* checa se os GPIOs estao prontos e deixa tudo desligado.
 * Pode chamar de novo sem medo */
int ledmux_init(void);

/* quantos LEDs tem no devicetree */
size_t ledmux_count(void);

/* a função "geral": qualquer ação em um LED, ou em todos com LEDMUX_ALL */
int ledmux_apply(size_t led, enum ledmux_action action);

/* só um atalho pro apply com LEDMUX_ALL */
int ledmux_apply_all(enum ledmux_action action);

/* atalhos, todos aceitam LEDMUX_ALL também */
int ledmux_on(size_t led);
int ledmux_off(size_t led);
int ledmux_toggle(size_t led);
int ledmux_blink(size_t led, enum ledmux_pattern pattern);

/* lê o nivel atual (0 ou 1), usei no main pra imprimir.
 * -EINVAL se o indice for ruim */
int ledmux_get(size_t led);

#endif /* LEDMUX_H */
