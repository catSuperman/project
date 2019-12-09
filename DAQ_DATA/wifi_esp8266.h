#ifndef __ESP8266_H_
#define __ESP8266_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>
//#include <asm/ioctls.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
//#include "systemtimer.h"
#include "DataTypes.h"
//#include "control_lnterface.h"
#include "DBC_SignalExplain.h"

extern int fd_wifi;



#ifdef __cplusplus
extern "C"{
#endif

//打开设�??
int openSerialWifi(char *cSerialName);
//发送指�?
void sendCmdWifi(char *str);
void sendCmdWifi2(unsigned char *str);
//wifi设置
void Set_Wifi(void);

#ifdef __cplusplus
}

#endif
#endif
