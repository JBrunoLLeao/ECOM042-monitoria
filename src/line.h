#ifndef LINE_H
#define LINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CONFIG_MINISHELL_LINE_MAX
#define CONFIG_MINISHELL_LINE_MAX 64
#endif

enum line_ev {
	LINE_NONE,     /* nada a fazer */
	LINE_CHAR,     /* caractere aceito: ecoar */
	LINE_ERASE,    /* apagou um caractere: ecoar "\b \b" */
	LINE_DONE,     /* linha pronta em l->buf */
	LINE_TOO_LONG, /* linha descartada */
};

struct line {
	char buf[CONFIG_MINISHELL_LINE_MAX + 1];
	size_t len;
	bool overflow;
	bool last_cr;
};

void line_init(struct line *l);
enum line_ev line_feed(struct line *l, uint8_t c);

#endif
