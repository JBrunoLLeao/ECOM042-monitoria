#include "line.h"

void line_init(struct line *l)
{
	l->len = 0;
	l->overflow = false;
	l->last_cr = false;
	l->buf[0] = '\0';
}

enum line_ev line_feed(struct line *l, uint8_t c)
{
	bool was_cr = l->last_cr;

	l->last_cr = (c == '\r');

	if (c == '\n' && was_cr) {
		return LINE_NONE; /* par \r\n conta como uma linha so */
	}
	if (c == '\r' || c == '\n') {
		enum line_ev ev = l->overflow ? LINE_TOO_LONG : LINE_DONE;

		l->buf[l->len] = '\0';
		l->len = 0;
		l->overflow = false;
		return ev;
	}
	if (c == 0x08 || c == 0x7F) {
		if (!l->overflow && l->len > 0) {
			l->len--;
			return LINE_ERASE;
		}
		return LINE_NONE;
	}
	if (c < 0x20) {
		return LINE_NONE;
	}
	if (l->overflow) {
		return LINE_NONE;
	}
	if (l->len >= CONFIG_MINISHELL_LINE_MAX) {
		l->overflow = true;
		return LINE_NONE;
	}
	l->buf[l->len++] = (char)c;
	return LINE_CHAR;
}
