
#ifndef __SYSTEM_TIMER_H_
#define __SYSTEM_TIMER_H_

#include <unistd.h>
#include "DataTypes.h"


#define ERR_INDEX_RTC_INIT      0
#define ERR_INDEX_GPIO_INIT     1
#define ERR_INDEX_DAQ_CFG_INIT  2
#define ERR_INDEX_MSG_LOG_INIT  3
#define ERR_INDEX_DAQ_CAL_INIT       4
#define ERR_INDEX_GPS_INIT        5
#define ERR_INDEX_DAQ_CAL_RX_ODT       6

extern unsigned int g_GlobalErrorCode;

void SetError(unsigned char index,boolean bActive);
boolean IsErrorActive(unsigned char index);


#define DAQ_RECV_MSG_TIME_OUT_MAX   0  // 5秒


void InitSystemTimer();
double GetSystemTimeMs();


#endif