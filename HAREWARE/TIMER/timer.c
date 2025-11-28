#include "timer.h"
#include "stm32f10x.h"


extern int sendDataTime;

// 喂食计时
extern int feedTime;
extern int cacheSendTime;
	
extern int feedFlag;
extern int sendFlag;
extern int feedEndFlag;
extern u16 USART_RX_STA;    


#define SG90_CLOSE  175    // 舵机关闭
#define SG90_OPEN   185    // 舵机打开

int SG90_OpenTime = 2;     // 舵机开启时间
int cacheOpenTIme = 2;     // 缓存舵机开启时间
// 定时器2
// arr：自动重装值 psc：时钟预分频数 CH3 -> PA2
void timePwmInit( u16 arr,u16 psc ){
	
	
  TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA| RCC_APB2Periph_AFIO,ENABLE); 
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3;                // PA2 PA3			
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	         // 设置GPIO为推挽输出模式	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 
	GPIO_Init(GPIOA, &GPIO_InitStructure);	

	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = arr;   
	TIM_TimeBaseInitStructure.TIM_Prescaler = psc; 
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure); 

	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;      // 输出极性选择
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 输出状态使能
	TIM_OCInitStructure.TIM_Pulse = 175;    
  
	TIM_OC3Init(TIM2,&TIM_OCInitStructure);
	TIM_OC4Init(TIM2,&TIM_OCInitStructure);
	
  TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);		 //启用CCR3上的TIM2外围预加载寄存器。
  TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);		 //启用CCR3上的TIM2外围预加载寄存器。
	
	TIM_ARRPreloadConfig(TIM2,ENABLE);//ARPE使能
	TIM_Cmd(TIM2, ENABLE);               
							 
}

// 定时器3
void timeInit(u16 arr,u16 psc){
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能APB1

	TIM_TimeBaseStructure.TIM_Period = arr; 
	TIM_TimeBaseStructure.TIM_Prescaler = psc;  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 
 
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	TIM_Cmd(TIM3, DISABLE);  //使能TIMx外设 定时器使能						 
}

void TIM3_IRQHandler(void)   //TIM3中断
{ 
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{
			USART_RX_STA |= 1<<15;	//标记接收完成
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除TIMx的中断待处理位:TIM 中断源 
			TIM_Cmd(TIM3, DISABLE);  //关闭TIM3
		}
}


void timeSendInit(u16 arr,u16 psc){
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //时钟使能APB1

	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 计数到5000为500ms
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值  10Khz的计数频率  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
 
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);  

	TIM_Cmd(TIM4, ENABLE);  //使能TIMx外设 定时器使能
							 
}

void TIM4_IRQHandler(void)   //TIM4中断
{ 
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{
			// 数据发送计时
			sendDataTime--;
			if( !sendDataTime )sendFlag = 1;;
			
			// 喂食计时 == 0 到达倒计时
			if( feedTime ){
				feedTime--;
			}else{
				// 开始喂食
				feedFlag = 1;
				// 喂食结束(用if，时间为有效数才减，防止数据越界)
				if( SG90_OpenTime ){
					SG90_OpenTime--;	
				}else{
					feedEndFlag = 1;
					SG90_OpenTime = cacheOpenTIme;
				}				
			}
			TIM_SetCounter(TIM4,0);
			TIM_ClearITPendingBit(TIM4, TIM_IT_Update); 
		}
}



