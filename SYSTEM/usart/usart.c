#include "sys.h"
#include "led.h"
#include "usart.h"	  
#include "delay.h"
#include "oled.h"
#include <string.h>
#include <stdio.h>
#include "usart2.h"

// 处理数据的缓冲区
char WIFICon[256],UserName[256],IP[256],SubTopic[256],PubTopic[1024],Data[256];
char *USARTx_RX_BUF;

// 数据存储
extern int connectFlag;
extern int handleFlag;
extern int initFlag;            // esp8266初始化状态
//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 


 
#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  
void delay(int time){
	while( time ){
		delay_ms(1);
		time--;
	}
}  

void uart_init(u32 bound){
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
  
	//USART1_TX   GPIOA.9
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.9
   
  //USART1_RX	  GPIOA.10初始化
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//PA10
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.10  

  //Usart1 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式


  USART_Init(USART1, &USART_InitStructure); //初始化串口1
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启串口接受中断
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//开启串口接受中断
	
  USART_Cmd(USART1, ENABLE);                    //使能串口1 

}

void USART1_IRQHandler(void){
	u8 res;
	char *res1,*res2,*res3,*res4,*res5,*res6,*res7,*res8,*res9;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
		res = USART_ReceiveData(USART1);	
		if( USART_RX_STA < 400 ){
			TIM_SetCounter(TIM3,0);                   // 计数器清空 
			if(USART_RX_STA == 0 && !initFlag) 				// 使能定时器3的中断 初始化完毕后不需要开启定时器
			{
				TIM_Cmd(TIM3,ENABLE);                   // 使能定时器3
			}
			USART_RX_BUF[USART_RX_STA++] = res;	      // 记录接收到的值	 
		}  		 
   } 
	
	 if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET){
		 // 初始化完成后才能执行下面的操作
		 if( initFlag ){  
		   USARTx_RX_BUF = strstr((const char * )USART_RX_BUF,(const char * )"{\"");
		    
		   res1 = strstr((const char * )USARTx_RX_BUF,(const char * )"\"LED\":1");
		   res2 = strstr((const char * )USARTx_RX_BUF,(const char * )"\"LED\":0");
		   res3 = strstr((const char * )USARTx_RX_BUF,(const char * )"\"FAN\":1");
		   res4 = strstr((const char * )USARTx_RX_BUF,(const char * )"\"FAN\":0");
		   res5 = strstr((const char * )USARTx_RX_BUF,(const char * )"E_TEMP");
		   res6 = strstr((const char * )USARTx_RX_BUF,(const char * )"E_HUM");
		   res7 = strstr((const char * )USARTx_RX_BUF,(const char * )"E_SUN");
		   res8 = strstr((const char * )USARTx_RX_BUF,(const char * )"E_CO2");
		   res9 = strstr((const char * )USARTx_RX_BUF,(const char * )"E_SOIL");			 
			 
		   if( res1 ) handleFlag = 1; 
		   if( res2 ) handleFlag = 2;
			 if( res3 ) handleFlag = 3; 
		   if( res4 ) handleFlag = 4; 			 
		   
		   if( res5 ) handleFlag = 5;  // {"E_TEMP":1,"temp_up":20,"temp_down":20}		
		   if( res6 ) handleFlag = 6;  // {"E_HUM":1,"hum_up":20,"hum_down":20}		
		   if( res7 ) handleFlag = 7;	 // {"E_SUN":1,"sun_up":20,"sun_down":20}		
		   if( res8 ) handleFlag = 8;  // {"E_CO2":1,"res_up":20,"res_down":20}
			 if( res9 ) handleFlag = 9;  // {"E_SOIL":1,"soil_up":70,"soil_down":10}
			 

			 USART_RX_STA = 0;
		 }
		 		
		USART_ClearFlag(USART1, USART_FLAG_IDLE); // 清除空闲中断标志位
		USART1->SR; // 读取SR寄存器
		USART1->DR; // 读取DR寄存器
	 }	 
	 
} 
#endif	

// 清除串口缓存
void clearCache( void ){
	int i;
	for( i=0;i<200;i++ ){
	 USART_RX_BUF[i] = 0;
	}
}


int waitDataStatus(char*atData,char *data,int waitTime){
	char *res;
	USART_RX_STA=0;
	printf(atData);
	while(--waitTime)
	{
		delay_ms(1);
		if(USART_RX_STA & 0X8000)//接收到一次数据
		{
			USART_RX_STA = 0;
			res = strstr((const char * )USART_RX_BUF,(const char * )data);
			if(res) return 1;	
		}
	}
	return 0;
}

void ESP8266Init(char* WIFIname,char* WIFIpwd){
	
	
	OLED_ShowString(35,42,"Loading",16,1);OLED_Refresh();
	while(!waitDataStatus("AT+RST\r\n","OK",5000));
	delay_ms(1000);
	
	while(!waitDataStatus("AT+CWMODE=2\r\n","OK",5000));
	delay_ms(1000);
	OLED_ShowString(35,42,"Loading .",16,1);OLED_Refresh();
	
	sprintf(WIFICon,"AT+CWSAP=\"%s\",\"%s\",5,3\r\n",WIFIname,WIFIpwd);
	while(!waitDataStatus(WIFICon,"OK",5000));
	delay_ms(1000);
	
	OLED_ShowString(35,42,"Loading ..",16,1);OLED_Refresh();
	sprintf(UserName,"AT+CIPMUX=1\r\n");
	while(!waitDataStatus(UserName,"OK",5000));
	delay_ms(1000);
	
	sprintf(IP,"AT+CIPSERVER=1,8080\r\n");
	while(!waitDataStatus(IP,"OK",15000));
	
	memset(USART_RX_BUF,0,sizeof(USART_RX_BUF));
	USART_RX_STA = 0; initFlag = 1;   // 初始化完成
}

void ESP8266Pub(int temperature,int waterLevel,int waterTds){
	int length;
	
	sprintf(PubTopic,"{\"Temperature\":%d,\"waterLevel\":%d,\"waterTds\":%d}\r\n",temperature,waterLevel,waterTds);
	length = strlen(PubTopic);  // 获取字符串长度（不包括 '\0'）
	sprintf(Data,"AT+CIPSEND=0,%d\r\n",length);
	printf(Data);
	delay_ms(350);
	printf(PubTopic);
}







