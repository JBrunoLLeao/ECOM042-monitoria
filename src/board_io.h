/*******************************************************************
 * @file board_io.h
 *
 * @brief Interface do módulo de entradas/saídas (Atividade-03).
 *******************************************************************/

#ifndef BOARD_IO_H_
#define BOARD_IO_H_

#include <stdbool.h>

#include <zephyr/devicetree.h>

/* O native_sim só declara o LED (alias led0). O botão (alias sw0) precisa
 * vir de um overlay de devicetree criado por você. */
#if !DT_NODE_EXISTS(DT_ALIAS(sw0))
#error "Alias sw0 nao existe: crie um overlay de devicetree descrevendo o botao."
#endif

/* Configura LED (saída, iniciando desligado) e botão (entrada), obtidos do
 * devicetree pelos aliases led0 e sw0. Retorna 0 ou errno negativo. */
int io_init(void);

/* Escreve o nível lógico do LED (ativo em alto). Retorna 0 ou errno negativo. */
int led_set(bool on);

/* Lê o botão: 1 pressionado, 0 solto, ou errno negativo. */
int button_read(void);

#endif /* BOARD_IO_H_ */