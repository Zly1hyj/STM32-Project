#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f10x.h"

void timeInit(u16 arr,u16 psc);
void timeSendInit(u16 arr,u16 psc);
void timePwmInit( u16 arr,u16 psc );
void time1PwmInit( u16 arr,u16 psc );


#endif


