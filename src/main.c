/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include <zephyr/kernel.h>

#include "command.h"
#include "commands.h"

int main(void)
{
	/* TODO (Atividade-02): criar command.{h, c} e commands.{h, c}
	 * (Command Pattern), montar a tabela de comandos e despachar.
	 */
	dispatch_command(commands_table, commands_table_size, "motor_on");
	dispatch_command(commands_table, commands_table_size, "motor_off");
	dispatch_command(commands_table, commands_table_size, "increase_speed");
	dispatch_command(commands_table, commands_table_size, "decrease_speed");
	dispatch_command(commands_table, commands_table_size, "teste_erro"); 
	return 0;
}