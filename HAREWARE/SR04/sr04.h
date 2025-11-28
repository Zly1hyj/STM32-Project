#ifndef __SR04_H
#define __SR04_H

#include "stm32f10x.h"

void HC_SR04Config(void);
void Open_Tim2(void);
void Close_Tim2(void);
int GetEcho_time(void);
float Getlength(void);

#define TRIG_Send(a)   if(a)\
											 GPIO_SetBits(GPIOB,GPIO_Pin_8);\
											 else\
											 GPIO_ResetBits(GPIOB,GPIO_Pin_8)
        			
#define ECHO_Reci GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_9)				
  

#endif
