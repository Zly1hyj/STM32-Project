#include "adc.h"
#include "delay.h"
#include <math.h>


float TDS_value = 0.0,voltage_value;
float compensationCoefficient = 1.0;//温度校准系数
float compensationVolatge;
float kValue = 1.67;

extern unsigned char temp;

// 用于保存转换计算后的电压值 	 
float TDS; 

void Adc_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;                             
	ADC_InitTypeDef  ADC_InitStructure;                              
																																					 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE );	              
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE );	               
																																					 
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);                                     // 设置ADC分频因子6 72M/6=12,ADC最大时间不能超过14M
																																					 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;                // 准备设置PB0
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		                      // 模拟输入引脚
	GPIO_Init(GPIOB, &GPIO_InitStructure);                                // 设置PB0
																																					 
	ADC_DeInit(ADC1);                                                     // 复位ADC1,将外设 ADC1 的全部寄存器重设为缺省值
																																					 
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	                  // ADC工作模式:ADC1和ADC2工作在独立模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;	                        // 模数转换工作在单通道模式
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;	                  // 模数转换工作在单次转换模式
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	  // 转换由软件而不是外部触发启动
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	              // ADC数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = 1;	                              // 顺序进行规则转换的ADCStructure);                                   // 根据ADC_InitStruct中指定的参数初始化外设ADCx的寄存器   
	ADC_Init(ADC1, &ADC_InitStructure);  
	
	ADC_Cmd(ADC1, ENABLE);	                                              // 使能指定的ADC1	
	ADC_ResetCalibration(ADC1);	                                          // 使能复位校准  	 
	while(ADC_GetResetCalibrationStatus(ADC1));                     	    // 等待复位校准结束	
	ADC_StartCalibration(ADC1);	                                          // 开启AD校准
	while(ADC_GetCalibrationStatus(ADC1));	              
	
}




int Get_Adc(int ch)   
{	
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	// ADC1,ADC通道,采样时间为239.5周期	  			    
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		                          // 使能指定的ADC1的软件转换启动功能	
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));                     // 等待转换结束
	return ADC_GetConversionValue(ADC1);	                              // 返回最近一次ADC1规则组的转换结果
}

/*-------------------------------------------------*/
/*函数名：平均多次ADC结果，提高精度                  */
/*参  数：channel: 通道数                           */
/*参  数：count: 平均次数                           */
/*-------------------------------------------------*/	
int Get_Adc_Average(int channel,int count)
{
	int sum_val=0,temp_avrg = 0;
	char t;
	
	for(t=0;t<count;t++)              //循环读取times次
	{
		sum_val += Get_Adc(channel);    //计算总值
		delay_ms(5);                    //延时
	}
	temp_avrg = sum_val / count;

 return temp_avrg;             // 返回平均值
} 

/**************TDS值采集函数***************/
void TDS_Value_Conversion( void )
{
	int tds,adcValue;
	
	adcValue = Get_Adc_Average(8,5);
	
	TDS = (float)adcValue / 4096 * 3.3; //AD转换
	tds = 66.71*TDS*TDS*TDS - 127.93*TDS*TDS + 428.7*TDS;
	TDS_value = tds;
	if((TDS_value<=0)){TDS_value=0;}
	if((TDS_value>1400)){TDS_value=1400;}	
}
