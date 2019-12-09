#ifndef __RTC_H_
#define __RTC_H_

#include "DataTypes.h"

boolean InitRtcTime();
boolean GetRtcTime(unsigned char *rx_buf);
boolean SetRtcTime(unsigned char *cst);
void ExitRtcTime();

void utc2cst(unsigned char *utc,unsigned char *cst);


#endif