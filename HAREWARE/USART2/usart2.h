#ifndef __USART2_H
#define __USART2_H
#include "stm32f10x.h"

void uart2_init( u32 bound );
int getCo2Density( void );
void USART_SendString(USART_TypeDef* USARTx, char *DataString);
#endif


