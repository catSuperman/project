#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <termios.h>
#include <errno.h>  
#include "rtc.h"

#define I2C_SLAVE	0x0703
#define I2C_TENBIT	0x0704

#define I2C_ADDR  0xD0
#define RTC_DATA_LEN 9

const char year_monthdays[2][13] = {{0,31,28,31,30,31,30,31,31,30,31,30,31}
									,{0,31,29,31,30,31,30,31,31,30,31,30,31}};

pthread_mutex_t g_MutexOperatorRtcTime;

boolean InitRtcTime()
{
    pthread_mutex_init(&g_MutexOperatorRtcTime, NULL);
	return true;
}

void ExitRtcTime()
{
    pthread_mutex_destroy(&g_MutexOperatorRtcTime);
}

boolean GetRtcTime(unsigned char *rx_buf)
{
    unsigned int uiRet;
	int nRtcI2cFd = 0;
	unsigned char addr[2] = {0};

	pthread_mutex_lock(&g_MutexOperatorRtcTime); 
	nRtcI2cFd = open("/dev/i2c-1", O_RDWR);   
	if(nRtcI2cFd < 0)
	{
        pthread_mutex_unlock(&g_MutexOperatorRtcTime); 
        printf("Rtc Time: open i2c-1 failed \n");
		return false;
	}

  	uiRet = ioctl(nRtcI2cFd, I2C_SLAVE, I2C_ADDR >> 1);
    if (uiRet < 0) 
    {
        close(nRtcI2cFd);
        pthread_mutex_unlock(&g_MutexOperatorRtcTime);
		printf("Rtc Time: ioctl failed 0x%X\n",uiRet);
		return false;
    }

	write(nRtcI2cFd, addr, 1);
	read(nRtcI2cFd, rx_buf, RTC_DATA_LEN - 1);
	close(nRtcI2cFd);
	pthread_mutex_unlock(&g_MutexOperatorRtcTime); 

	return true;
}

char int2bcd(int num)
{
	char result;
	result = 0xF0 & ((num/10)<<4);
	result += (0x0F&(num%10));
	return result;
}

boolean SetRtcTime(unsigned char *cst)
{
	int nRtcI2cFd = 0;
	unsigned int uiRet,i;
	unsigned char tx_buf[RTC_DATA_LEN] = {0};
	unsigned char addr[2] = {0};
	addr[0] = 0x00;
	tx_buf[0] = 0x00;
	printf("set time 3 \n");
	//printf("localtime in fun: %02x %02x %02x %02x %02x \n",localtime[0],localtime[1],localtime[2],localtime[3],localtime[4],localtime[5]);

	tx_buf[7] = int2bcd(cst[0]); //year
	tx_buf[6] = int2bcd(cst[1]); //mounth
	tx_buf[5] = int2bcd(cst[2]); //date
	tx_buf[4] = 0x07 & 1; //day
	tx_buf[3] = int2bcd(cst[3]); tx_buf[3] += 0x80; //hour 
	tx_buf[2] = int2bcd(cst[4]); //minute
	tx_buf[1] = int2bcd(cst[5]); //secend

	pthread_mutex_lock(&g_MutexOperatorRtcTime); 
	nRtcI2cFd = open("/dev/i2c-1", O_RDWR);   
	if(nRtcI2cFd < 0)
	{
        pthread_mutex_unlock(&g_MutexOperatorRtcTime); 
        printf("Rtc Time: open i2c-1 failed \n");
		return false;
	}

  	uiRet = ioctl(nRtcI2cFd, I2C_SLAVE, I2C_ADDR >> 1);
	if (uiRet < 0) 
	{
        close(nRtcI2cFd);
        pthread_mutex_unlock(&g_MutexOperatorRtcTime);
		printf("Rtc Time: ioctl failed 0x%X\n",uiRet);
		return false;
	}
	write(nRtcI2cFd, tx_buf, 8);
	close(nRtcI2cFd);
	pthread_mutex_unlock(&g_MutexOperatorRtcTime);	
	return true;	
}

void utc2cst(unsigned char *utc,unsigned char *cst)
{
	volatile unsigned char year,mounth,date,hour,minute,second;
	unsigned char isLeapYear;
	
	year   = utc[0];
	mounth = utc[1];
	date   = utc[2];
	hour   = utc[3];
	minute = utc[4];
	second = utc[5];
	
	if(year % 4)
		isLeapYear = 0;
	else
		isLeapYear = 1;	

	hour += 8;

	if(hour >= 24)
	{
		hour -= 24;
		date++;
		
		if(date > year_monthdays[isLeapYear][mounth])
		{
			date = 1;
			mounth++;
			
			if(mounth > 12)
			{
				mounth = 1;
				year++;
			}
		}
	}
	cst[0] = year;
	cst[1] = mounth;
	cst[2] = date;
	cst[3] = hour;
	cst[4] = minute;
	cst[5] = second;	
}