#include <string.h>
#include <zephyr/sys/printk.h>

#include "command.h"

int dispatch_command(const struct command_entry *table, size_t table_size, const char *name)
{
	for (size_t i = 0; i < table_size; i++) {
		if (strcmp(table[i].name, name) == 0) {
			table[i].execute();
			return 0;
		}
	}

	printk("comando desconhecido: \"%s\"\n", name);
	return -1;
}