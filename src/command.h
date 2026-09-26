#ifndef COMMAND_H_
#define COMMAND_H_

#include <stddef.h>

typedef struct command_entry {
	const char *name;
	void (*execute)(void);
} command_entry;

/*
Recebe uma tabela de comandos (array de struct command_entry), o
tamanho dessa tabela, e o nome do comando a executar.
 */
int dispatch_command(const struct command_entry *table, size_t table_size, const char *name);

#endif