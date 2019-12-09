#ifndef __4g_SIM7600_H_
#define __4g_SIM7600_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>
#include <asm/ioctls.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "systemtimer.h"

#include "DataTypes.h"

#include <stdio.h>
#include <string.h>
//#include "setupmenu.h"


extern char serverip[16]; //每行最大读取字符数
extern char portnumber[5]; //每行最大读取字符数

extern int fd_4g;
#ifdef __cplusplus
extern "C"{
#endif
//打开串口
int openSerial4g(char *cSerialName);
//发送命令
void sendCmd4g(char *str);
void sendCmd4g_byte(unsigned char *str);
//模块初始化
void Set_lte(void);




#ifdef __cplusplus
}

#endif

#endif
