#ifndef CMD_H
#define CMD_H

#include <stdbool.h>
#include <stddef.h>

/* Hardware injetado: o modulo nao conhece GPIO. */
struct cmd_hw {
	int (*led_set)(bool on);
	int (*led_get)(void);
	int (*btn_get)(void);
};

/* Executa uma linha e escreve a resposta (sem \r\n) em out. Retorna 0 se ok. */
int cmd_execute(const char *line, const struct cmd_hw *hw, char *out, size_t out_len);

#endif
