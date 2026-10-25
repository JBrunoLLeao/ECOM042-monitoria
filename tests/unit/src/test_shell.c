#include "cmd.h"
#include "line.h"
#include <string.h>
#include <zephyr/ztest.h>

static bool led_state, btn_state;

static int f_set(bool on)
{
	led_state = on;
	return 0;
}

static int f_get(void)
{
	return led_state;
}

static int f_btn(void)
{
	return btn_state;
}

static const struct cmd_hw hw = {f_set, f_get, f_btn};
static char out[128];

static enum line_ev feed_str(struct line *l, const char *s)
{
	enum line_ev ev = LINE_NONE;

	while (*s) {
		ev = line_feed(l, (uint8_t)*s++);
	}
	return ev;
}

ZTEST(cmd, test_help)
{
	zassert_equal(cmd_execute("help", &hw, out, sizeof(out)), 0);
	zassert_true(strstr(out, "LED") != NULL);
}

ZTEST(cmd, test_led)
{
	led_state = false;
	zassert_equal(cmd_execute("LED ON", &hw, out, sizeof(out)), 0);
	zassert_true(led_state);
	zassert_equal(cmd_execute("led", &hw, out, sizeof(out)), 0);
	zassert_str_equal(out, "LED ON");
	cmd_execute("Led off", &hw, out, sizeof(out));
	zassert_false(led_state);
	zassert_not_equal(cmd_execute("LED XX", &hw, out, sizeof(out)), 0);
}

ZTEST(cmd, test_btn_echo_invalid)
{
	btn_state = true;
	cmd_execute("BTN", &hw, out, sizeof(out));
	zassert_str_equal(out, "BTN PRESSED");
	cmd_execute("ECHO Oi Mundo", &hw, out, sizeof(out));
	zassert_str_equal(out, "Oi Mundo");
	zassert_not_equal(cmd_execute("FOO", &hw, out, sizeof(out)), 0);
	zassert_str_equal(out, "ERR: unknown command");
}

ZTEST(cmd, test_null_callbacks)
{
	struct cmd_hw empty = {0};

	zassert_not_equal(cmd_execute("LED ON", &empty, out, sizeof(out)), 0);
	zassert_not_equal(cmd_execute("BTN", &empty, out, sizeof(out)), 0);
	zassert_not_equal(cmd_execute("LED ON", NULL, out, sizeof(out)), 0);
	zassert_not_equal(cmd_execute(NULL, &hw, out, sizeof(out)), 0);
}

ZTEST(line, test_done_and_backspace)
{
	struct line l;

	line_init(&l);
	feed_str(&l, "LEX");
	zassert_equal(line_feed(&l, 0x7F), LINE_ERASE);
	feed_str(&l, "D");
	zassert_equal(line_feed(&l, '\r'), LINE_DONE);
	zassert_str_equal(l.buf, "LED");
	zassert_equal(line_feed(&l, '\n'), LINE_NONE); /* \r\n = uma linha */
}

ZTEST(line, test_too_long)
{
	struct line l;

	line_init(&l);
	for (int i = 0; i < CONFIG_MINISHELL_LINE_MAX + 10; i++) {
		line_feed(&l, 'a');
	}
	zassert_equal(line_feed(&l, '\n'), LINE_TOO_LONG);

	/* proxima linha funciona normalmente */
	feed_str(&l, "HELP");
	zassert_equal(line_feed(&l, '\n'), LINE_DONE);
	zassert_str_equal(l.buf, "HELP");
}

ZTEST(line, test_exact_max)
{
	struct line l;

	line_init(&l);
	for (int i = 0; i < CONFIG_MINISHELL_LINE_MAX; i++) {
		line_feed(&l, 'a');
	}
	zassert_equal(line_feed(&l, '\n'), LINE_DONE);
}

ZTEST_SUITE(cmd, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(line, NULL, NULL, NULL, NULL, NULL);
