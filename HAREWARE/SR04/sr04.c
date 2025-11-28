#include "sr04.h"
#include "delay.h"

uint16_t mscount = 0;//定义毫秒级计数

int mathFlag = 0;
float sum = 0; 
float newLength = 0.0;

void HcsrTimeInit(u16 arr,u16 psc){
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);  // 开启TIM1时钟

	TIM_TimeBaseStructure.TIM_Period = arr; 
	TIM_TimeBaseStructure.TIM_Prescaler = psc;  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); 
 
	TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	TIM_Cmd(TIM1, DISABLE);  //使能TIMx外设 定时器使能						 
}

void HC_SR04Config(void)
{
	  GPIO_InitTypeDef GPIO_hcsr04Init;//超声波时钟结构体初始化


	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);//打开GPIO时钟

	  //Trig PC14 输出端 高电平
	  GPIO_hcsr04Init.GPIO_Mode  = GPIO_Mode_Out_PP;//推挽输出
	  GPIO_hcsr04Init.GPIO_Pin   = GPIO_Pin_8;//引脚14
	  GPIO_hcsr04Init.GPIO_Speed = GPIO_Speed_50MHz;//速度为50Mhz     
	  GPIO_Init(GPIOB,&GPIO_hcsr04Init);//配置GPIO初始化函数
	
	  //Echo PC15 输入端 
	  GPIO_hcsr04Init.GPIO_Mode  = GPIO_Mode_IN_FLOATING;//浮空输入
	  GPIO_hcsr04Init.GPIO_Pin   = GPIO_Pin_9;          //引脚15
	  GPIO_Init(GPIOB,&GPIO_hcsr04Init);                 //配置GPIO初始化函数
		HcsrTimeInit(999,71);	                             // 定时1ms                   
}


void Open_TIM1(void)//定时器开启
{
		TIM_SetCounter(TIM1,0);//开启定时器
	  mscount = 0;
	  TIM_Cmd(TIM1,ENABLE);  //打开定时器
}

void Close_TIM1(void)     //定时器关闭
{
		TIM_Cmd(TIM1,DISABLE);//失能定时器
}


void TIM1_UP_IRQHandler(void)//中断服务函数（判断是否发生中断）
{
		if(TIM_GetITStatus(TIM1,TIM_IT_Update)!=RESET)
		{
			mscount++;
			TIM_ClearITPendingBit(TIM1,TIM_IT_Update);//清除中断标志位
				 
		}
}


int GetEcho_time(void)//获取定时器的数值
{
		uint32_t t=0;
	  t = mscount * 1000;//中断时间
	  t+= TIM_GetCounter( TIM1 );//得到定时器计数时间
	  TIM1 -> CNT = 0;//重装载值为0
	  delay_ms(10);   //延迟50ms	
	  return t;
}

float Getlength(void)//获取距离长度
{
	uint32_t t = 0;    // 定义时间t
	float length = 0;  // 定义长度length

	if((mathFlag++) >= 3){
		newLength = sum /= 3.0;       //计算距离平均值
		sum = 0;mathFlag = 0;
	}else{
			TRIG_Send(1); // 发送超声波  
		  delay_us(30); // 发送20us
		  TRIG_Send(0); // 停止发送超声波
	
		  while(ECHO_Reci == 0);// 当超声波发出后
			Open_TIM1();          // 打开定时器
						
			while(ECHO_Reci == 1); // 当收到超声波返回信号
			Close_TIM1();          // 关闭定时器
			t = GetEcho_time();    // 获取定时器计数数值
		  
		 length=((float)t/58.0);// 计算出距离长度
		 sum = sum + length;    // 距离长度求和	
	}
	 // newLength = newLength > 300?300:newLength;
	 return newLength;
}
 




