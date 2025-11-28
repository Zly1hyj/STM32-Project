#include "usart2.h"
#include "delay.h"
#include "stm32f10x.h"


u8 USART2_RX_BUF[200];       // 接收缓冲,最大USART_REC_LEN个字节.
u16 USART2_RX_STA = 0;       // 接收状态标记


/**
void uart2_init( u32 bound )
{
	// GPIO端口设置
	GPIO_InitTypeDef	GPIO_InitStructure;
	USART_InitTypeDef	USART_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(  RCC_APB2Periph_GPIOA, ENABLE ); 
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2, ENABLE );        

	// PA2 TXD2 
	GPIO_InitStructure.GPIO_Pin	= GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF_PP;
	GPIO_Init( GPIOA, &GPIO_InitStructure );

	// PA3 RXD2 
	GPIO_InitStructure.GPIO_Pin	= GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN_FLOATING;
	GPIO_Init( GPIOA, &GPIO_InitStructure );

	// Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel			= USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 3;               
	NVIC_InitStructure.NVIC_IRQChannelSubPriority		= 2;                     
	NVIC_InitStructure.NVIC_IRQChannelCmd			= ENABLE;                      
	NVIC_Init( &NVIC_InitStructure );                                          

	// USART 初始化设置 
	USART_InitStructure.USART_BaudRate		= bound;                                
	USART_InitStructure.USART_WordLength		= USART_WordLength_8b;                  
	USART_InitStructure.USART_StopBits		= USART_StopBits_1;                    
	USART_InitStructure.USART_Parity		= USART_Parity_No;                     
	USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;       
	USART_InitStructure.USART_Mode			= USART_Mode_Rx | USART_Mode_Tx;       

	USART_Init( USART2, &USART_InitStructure );                                             
	USART_ITConfig( USART2, USART_IT_RXNE, ENABLE );  
	USART_ITConfig( USART2, USART_IT_IDLE, ENABLE );         	
	USART_Cmd( USART2, ENABLE );                                                           
}
**/

