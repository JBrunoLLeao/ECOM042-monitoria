#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "command.h"

/*comandos concretos*/

static void cmd_motor_on(void)
{
	printk("Motor ligado\n");
}

static void cmd_motor_off(void)
{
	printk("Motor desligado\n");
}

static void cmd_increase_speed(void)
{
	printk("Velocidade aumentada!\n");
}

static void cmd_decrease_speed(void)
{
	printk("Velocidade reduzida!\n");
}

/*
A tabela de comandos: mapeia nome -> função.
 */
const struct command_entry commands_table[] = {
	{ "motor_on",  cmd_motor_on  },
	{ "motor_off", cmd_motor_off },
	{ "increase_speed",  cmd_increase_speed },
	{ "decrease_speed",  cmd_decrease_speed },
};

const size_t commands_table_size = sizeof(commands_table) / sizeof(commands_table[0]);