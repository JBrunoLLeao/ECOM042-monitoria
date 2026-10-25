#include "cmd.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef int (*cmd_fn)(const struct cmd_hw *hw, const char *args, char *out, size_t len);

struct cmd_entry {
	const char *name;
	cmd_fn fn;
	const char *help;
};

static bool eq_nocase(const char *a, const char *b)
{
	while (*a && *b) {
		if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) {
			return false;
		}
	}
	return *a == *b;
}

static int cmd_help(const struct cmd_hw *hw, const char *args, char *out, size_t len)
{
	snprintf(out, len, "HELP | LED [ON|OFF] | BTN | ECHO <texto>");
	return 0;
}

static int cmd_led(const struct cmd_hw *hw, const char *args, char *out, size_t len)
{
	if (*args == '\0') {
		if (hw == NULL || hw->led_get == NULL) {
			snprintf(out, len, "ERR: led unavailable");
			return -1;
		}
		snprintf(out, len, "LED %s", hw->led_get() ? "ON" : "OFF");
		return 0;
	}
	bool on;

	if (eq_nocase(args, "ON")) {
		on = true;
	} else if (eq_nocase(args, "OFF")) {
		on = false;
	} else {
		snprintf(out, len, "ERR: usage: LED [ON|OFF]");
		return -1;
	}
	if (hw == NULL || hw->led_set == NULL) {
		snprintf(out, len, "ERR: led unavailable");
		return -1;
	}
	if (hw->led_set(on) < 0) {
		snprintf(out, len, "ERR: led failed");
		return -1;
	}
	snprintf(out, len, "OK");
	return 0;
}

static int cmd_btn(const struct cmd_hw *hw, const char *args, char *out, size_t len)
{
	if (hw == NULL || hw->btn_get == NULL) {
		snprintf(out, len, "ERR: button unavailable");
		return -1;
	}
	snprintf(out, len, "BTN %s", hw->btn_get() ? "PRESSED" : "RELEASED");
	return 0;
}

static int cmd_echo(const struct cmd_hw *hw, const char *args, char *out, size_t len)
{
	snprintf(out, len, "%s", args);
	return 0;
}

static const struct cmd_entry table[] = {
	{"HELP", cmd_help, "lista comandos"},
	{"LED", cmd_led, "liga/desliga/consulta LED"},
	{"BTN", cmd_btn, "estado do botao"},
	{"ECHO", cmd_echo, "repete o texto"},
};

int cmd_execute(const char *line, const struct cmd_hw *hw, char *out, size_t out_len)
{
	if (out == NULL || out_len == 0) {
		return -1;
	}
	out[0] = '\0';
	if (line == NULL) {
		return -1;
	}

	while (isspace((unsigned char)*line)) {
		line++;
	}
	if (*line == '\0') {
		return 0;
	}

	char name[16];
	size_t n = 0;

	while (line[n] && !isspace((unsigned char)line[n])) {
		n++;
	}
	if (n >= sizeof(name)) {
		snprintf(out, out_len, "ERR: unknown command");
		return -1;
	}
	memcpy(name, line, n);
	name[n] = '\0';

	const char *args = line + n;

	while (isspace((unsigned char)*args)) {
		args++;
	}

	for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
		if (eq_nocase(name, table[i].name)) {
			if (table[i].fn == NULL) { /* valida callback antes de executar */
				snprintf(out, out_len, "ERR: not implemented");
				return -1;
			}
			return table[i].fn(hw, args, out, out_len);
		}
	}
	snprintf(out, out_len, "ERR: unknown command");
	return -1;
}
