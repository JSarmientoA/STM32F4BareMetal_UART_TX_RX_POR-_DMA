

#ifndef UART_H_
#define UART_H_
#include <stdio.h> // Para poder usar printf()
#include "stm32f4xx.h"
void uart2_rx_tx_interrupcion_init(void);
void uart2_rx_tx_init(void);
char uart2_read(void);
void dma1_ch4_stream6(uint32_t origen,uint32_t destino,uint32_t longitud_dato);
#define SR_RXNE			(1U<<5)
#define HISR_TCIF6		(1U<<21)
#define HIFCR_TCIF6		(1U<<21)//0: evento sin transferencia completa en la transmisión x
                                //1: se produjo un evento de transferencia completa en la transmisión x
#endif /* UART_H_ */
