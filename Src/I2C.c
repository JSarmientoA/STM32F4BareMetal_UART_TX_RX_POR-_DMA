#include "stm32f4xx.h"
#define I2C1_EN					(1U<<21)
#define GPIOBEN					(1U<<1)
#define CR1_SWRST				(1U<<15)
#define CR2_FREQ				(1U<<4)
#define I2C_100KHZ				80//0B 0101 0000
#define SD_MODE_MAX_RISE_TIME	17
#define CR1_PE					(1U<<0)
#define SR2_BUSY				(1U<<1)
#define CR1_START				(1U<<8)
#define SR1_SB					(1U<<0)
#define SR1_ADDR				(1U<<1)
#define SR1_TxE					(1U<<7)
#define CR1_ACK					(10U<<1)
#define CR1_STOP				(1U<<9)
#define SR1_RxNE				(1U<<6)
#define SR1_BTF					(1U<<2)

/*Pinout
 * PB8 ---> SCL--> D15
 * PB9 ---> SDA--> D14
 * */
void I2C_int(void){

	/*Habilitar el reloj para el GPIOB*/
	RCC->AHB1ENR|= GPIOBEN;

	/*Estableser PB8 y PB9 como funcion alternativa*/
	GPIOB->MODER |=  (1U<<17);
	GPIOB->MODER &=~ (1U<<16);
	GPIOB->MODER |=  (1U<<19);
	GPIOB->MODER &=~ (1U<<18);

	/*Estableser PB8 y PB9 en open drain*/
	GPIOB->OTYPER |= (1U<<8);
	GPIOB->OTYPER |= (1U<<9);

	/*Habilite las resistencias pullup para PB9 y PB8*/
	GPIOB->PUPDR |=  (1U<<16);
	GPIOB->PUPDR &=~ (1U<<17);
	GPIOB->PUPDR |=  (1U<<18);
	GPIOB->PUPDR &=~ (1U<<19);

	/*Habilitar el reloj para el I2C*/
	RCC->APB1ENR |= I2C1_EN;

	/*Lo que queremos hacer es entrar en modo reset para configurar el I2C
	 * Esto se realiza en el registro de control I2C.*/

	/*I2C en estado de reset*/
	I2C1->CR1|= CR1_SWRST;

	/*Salida del reinicio I2C*/
	I2C1->CR1 &= ~CR1_SWRST;

	/*Estableser la frecuencia del bus donde esta conectado el I2C. Esto es:
	 * Bits 5:0 FREQ[5:0]: Frecuencia de reloj periférico, esto es:
	 * Los bits FREQ deben configurarse con el valor de frecuencia de reloj
	 * APB (periférico I2C conectado a APB). El periférico utiliza el campo
	 * FREQ para generar tiempos de configuración y retención de datos que
	 * cumplan con las especificaciones I2C. La frecuencia mínima permitida
	 * es de 2 MHz, la frecuencia máxima está limitada por la frecuencia máxima
	 * APB (45 MHz) y no puede exceder los 50 MHz (límite máximo intrínseco del
	 * periférico).
	 * */
	I2C1->CR2 |= CR2_FREQ;// 16MHZ

	/*Configuracion I2C*/

	/*Setear el I2C en modo estandar, esto es la configuracion del reloj*/
	I2C1->CCR = I2C_100KHZ;

	/*Setee el tiempo de riso*/
	I2C1->TRISE = SD_MODE_MAX_RISE_TIME;

	/*Habiltar el I2C*/
	I2C1->CR1 |= CR1_PE;
}

/*Esta funcion solo permite leer un dato no datos en rafaga*/
void I2C1_byRead(char DirEsclavo,char DirMemoriaEsclavo,char* Datos){
	volatile int tmp;
	/*Validar que el bus no este ocupado*/
	while(I2C1->SR2 & SR2_BUSY);

	/*Generando una condicion de imicio para el I2C*/
	I2C1->CR1 |= CR1_START;

	/*Esperar a que se establezca la bandera SB*/
	/*si el bit de inicio no está sentado,
	 *nos quedaremos atascados aquí, cuando se establezca SR1_SB, saldrá de este bucle aquí.*/
	while(!(I2C1->SR1 & SR1_SB));

	/*Transmitir la direccion al esclavo*/
	I2C1->DR = DirEsclavo<<1;

	/*Esperar hasta que se establezca la bandera de direccion SR1_ADDR*/
	while(!(I2C1->SR1 & SR1_ADDR));

	/*Se borra la bandera SR1_ADDR, para enviar una ubicación de
	 *memoria dentro de un esclavo que queremos leer.*/
	tmp = I2C1->SR2;

	/*Envía la direccion de memeria al esclavo.
	 *Luego esperamos hasta que el transmisor no esté vacío*/
	I2C1->DR = DirMemoriaEsclavo;

	/*Esperar hasta que el registro de datos esté vacío validando la vandera TXE*/
	while(!(I2C1->SR1 & SR1_TxE));

	/*Generar otra condición de inicio, denominada como condicion de reinicio*/
	I2C1->CR1 |= CR1_START;

	/*Esperar a que se establezca la bandera SB*/
	/*si el bit de inicio no está sentado,
	 *nos quedaremos atascados aquí, cuando se establezca SR1_SB, saldrá de este bucle aquí.*/
	/*siempre que tengamos la condición de arranque del generador. tenemos que esperar
	 *para que se establezca la bandera de inicio poeterioriormete enviamos la dirección del esclavo*/
	while(!(I2C1->SR1 & SR1_SB));

	/*Tansmitir la direccin de esclavo y se lee*/
	I2C1->DR << 1|1;

	/*Esperar hasta que se establezca la bandera de direccion SR1_ADDR.
	 *Para leer el esclavo esperamos hasta que se establezca la bandera de dirección*/
	while(!(I2C1->SR1 & SR1_ADDR));

	/*Desabilita el reconocomiento (Acknowledge).Luego deshabilitaríamos el reconocimiento
	 *y luego borramos la bandera de dirección*/
	I2C1->CR1 =~ CR1_ACK;

	/*Se borra la bandera SR1_ADDR*/
	tmp = I2C1->SR2;

	/*Se genra una condicion de prada*/
	I2C1->CR1 |= CR1_STOP;

	/*Esperamos hasta que se establezca la bandera RXNE. Esta banddra indica que
	 *el búfer de recepción no está vacío lo que significa que hay algo que hemos recibido*/
	while(!(I2C1->SR1 & SR1_RxNE));

	/*Se lee el registro de datos y almacena en la variable Datos.
	 *Esto es leemos los datos y los almacenamos en nuestra variable*/
	*Datos++ = I2C1->DR;
}

/*Esta funcion permite leer datos en rafaga donde n es el múmero de bytes a leer*/
void I2C1_burstRead(char DirEsclavo,char DirMemoriaEsclavo,int n,char* Datos){
	volatile int tmp;
	/*Tenemos que esperar hasta que el autobús no esté ocupado.*/
	while(I2C1->SR2 & SR2_BUSY);

	/*Generando una condicion de imicio para el I2C*/
	I2C1->CR1 |= CR1_START;

	/*A continuación. esperamos a que se establezca el bit de inicio.*/
	/*Esperar a que se establezca la bandera SB*/
	/*si el bit de inicio no está sentado,
	 *nos quedaremos atascados aquí, cuando se establezca SR1_SB, saldrá de este bucle aquí.*/
	while(!(I2C1->SR1 & SR1_SB));

	/*Entonces, una vez que esté configurado el bit SR1_SB, transmitiremos a la dirección esclava*/
	I2C1->DR = DirEsclavo<<1;

	/*Esperar hasta que se establezca la bandera de direccion SR1_ADDR*/
	while(!(I2C1->SR1 & SR1_ADDR));

	/*una vez que se establece la bandera de la dirección, vamos a borrar
	 *la bandera de la dirección leyendo el registro de estado SR2 y se envia su informacion  a tmp*/
	tmp = I2C1->SR2;

	/*A continuación, nos aseguraremos de que el transmisor esté vacío.*/
	while(!(I2C1->SR1 & SR1_RxNE));

	/*Tan pronto como esté vacío, enviaremos una dirección de memoria a la que queremos acceder.
	 *Entonces enviamos una dirección de memoria escribiéndola en el
	 *registro de datos utilizando la variale DirMemoriaEsclavo*/
	I2C1->DR = DirMemoriaEsclavo;

	/*luego esperamos nuevamente a que el transmisor este vacio.
	 *Es aconsejable esperar siempre a que un transmisor esté vacío antes de transmitir.*/
	while(!(I2C1->SR1 & SR1_RxNE));

	/*una vez hecho esto. enviaremos una condición de reinicio*/
	I2C1->CR1 |= CR1_START;

	/*A continuación, tenemos que esperar a que se establezca el bit de inicio
	 *inspeccionando el registro de estado SR1*/
	while(!(I2C1->SR1 & SR1_SB));

	/*Una vez hecho esto, vamos a transmitir la dirección del esclavo más la lectura.*/
	I2C1->DR << 1|1;

	/*Esperar hasta que se establezca la bandera de direccion SR1_ADDR*/
	while(!(I2C1->SR1 & SR1_ADDR));

	/*Una vez hecho esto, borramos la bandera de dirección*/
	tmp = I2C1->SR2;

	/*Una vez que hayamos borrado la bandera de dirección, habilitaremos el reconocimiento
	 *estableciendo el bit ACK en uno.*/
	I2C1->CR1 |= CR1_ACK;

	while(n > 0U){
		/*Si un byte, entonces si un byte*/
		if(n==1U){
			/*Desactivaríamos el reconocimiento.*/
			I2C1->CR1 &= ~CR1_ACK;

			/*Una vez que lo hayamos hecho, se generará una condición de parada.*/
			I2C1->CR1 |= CR1_STOP;

			/*Una vez que hayamos generado una condición de parada.
			 *Esperamos a que se establezca la bandera RXNE.
			 *En otras palabras, esperamos hasta que el búfer de recepción esté lleno*/
			while(!(I2C1->SR1 & SR1_RxNE));

			/*Luego leemos los datos del registro de datos.*/
			*Datos++ = I2C1->DR;

			break;
		}

		else{
			/*Esperar a que lleguen los datos inspeccionando el RXNE*/
			while(!(I2C1->SR1 & SR1_RxNE));

			/*Una vez echo esto vamos a almacenar nuestros datos en la variable Datos*/
			*Datos++ = I2C1->DR;

			/*Luego vamos a disminuir n.*/
			n--;
		}
	}

}

/*Esta funcion permite la escritura en rafaga donde n es el número de bytes.
 *Esta vez este último argumento (Datos) va a contener los datos queremos escribir
 *en el dispositivo esclavo I2C*/
void I2C1_burstWrite(char DirEsclavo,char DirMemoriaEsclavo,int n,char* Datos){
	volatile int tmp;
	/*Esperaremos hasta que el autobús no esté ocupado*/
	while(I2C1->SR2 & SR2_BUSY);

	/*Generar una condición de inicio, siempre comenzamos con una condición de inicio*/
	I2C1->CR1 |= CR1_START;

	/*Una vez hecho esto, esperamos a que se establezca la bandera de inicio en el
	 *registro de estado uno*/
	while(!(I2C1->SR1 & SR1_SB));

	/*Una vez hecho esto, una vez que se establece la bandera de inicio*/
	I2C1->DR = DirEsclavo<<1;

	/*Esperamos hasta que se establezca el indicador de dirección.*/
	while(!(I2C1->SR1 & SR1_ADDR));

	/*una vez que se establece la bandera de la dirección, vamos a borrar
	 *la bandera de la dirección leyendo el registro de estado SR2 y se envia su informacion  a tmp*/
	tmp = I2C1->SR2;

	/*Luego esperamos hasta que el registro de datos esté vacío.*/
#define SR1_RxNE				(1U<<6)

	/*Luego enviamos la dirección de memoria.*/
	I2C1->DR = DirMemoriaEsclavo;

	/*Este bucle transmitirá todos los bytes al I2C. una vez que se
	 *complete el bucle. tendríamos que esperar hasta que se establezca
	 *la bandera BTF.*/
	for(int i = 0; i < n;i++){

		/*esperamos hasta que el registro de datos esté vacío.
		 *una vez que está vacío, simplemente transmitimos el
		 *siguiente fragmento de datos*/
		while(!(I2C1->SR1 & SR1_TxE));

		I2C1->DR = *Datos++;
	}

	/*Esta bandera indica que la transferencia de bytes fue finalizada*/
	while(!(I2C1->SR1 & SR1_BTF));

	/*Una vez hecho esto, generaremos una condición de parada.*/
	I2C1->CR1 |= CR1_STOP;

}
