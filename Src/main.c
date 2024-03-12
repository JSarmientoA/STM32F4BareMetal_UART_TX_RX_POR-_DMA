#include <stdint.h>
#include <stdio.h> // Para poder usar printf()
#include "stm32f4xx.h"
#include "uart.h"

#define GPIOAEN		(1U<<0)// Escribir un uno(1) en el bit cero(0).
#define PIN5		(1U<<5)// Escribir un uno(1) en el bit 5.
#define	LED_PIN		PIN5

/*Este programa tranfiere la cadena de de caracteres por utilizando la uart */
static void dma1_callback(void);
int main(void)
{
	char mensaje[31] = "Transmision con STM32 por DMA\n\r";
	/*Habilitación del reoj para el puerto GPIOA*/
	RCC->AHB1ENR  |= 	GPIOAEN;//|=, es asignación con OR binario

	/*Estableser el pin PA5 como salida*/
	GPIOA->MODER  |= 	(1U<<10);//Coloca el bit 10 a 1
	GPIOA->MODER  &=~ 	(1U<<11);// Coloca   el bit 11 a 0, donde &=~ es Asignación con AND binario con complemto de uno

	uart2_rx_tx_init();
	dma1_ch4_stream6((uint32_t)mensaje,(uint32_t)&USART2->DR,31);// Donde el registro RD Contiene el carácter de datos recibidos o transmitidos,                                                                                 //dependiendo de si se lee o se escribe.
	while(1)
	{

	}
}

static void dma1_callback(void) {
	GPIOA->ODR|=LED_PIN;
}

void DMA1_Stream6_IRQHandler(void){
	/*Chequee si el bit TCIF6 esta seteado*/
	if(DMA1->HISR & HISR_TCIF6){
		/*Se borra la bandera de interrupcion*/
		DMA1->HIFCR |= HIFCR_TCIF6;
		/*Hacer algo*/
		dma1_callback();
		}
	}

