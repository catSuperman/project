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
#include <sys/ioctl.h>
#include "gpio.h"
//#include "network.h"



#define EDAS_POWERON 		11
#define EDAS_POWEROFF		10
#define EDAS_3G_WORK		20
#define EDAS_3G_OFF			21
#define EDAS_3G_START		31
#define EDAS_3G_STOP		30
#define EDAS_3G_RESTARTHI 	41
#define EDAS_3G_RESTARTLO	40

#define EDAS_LED_USBON		50
#define EDAS_LED_USBOFF		51

#define EDAS_LED_RXON		60
#define EDAS_LED_RXOFF		61

#define EDAS_LED_TXON		70
#define EDAS_LED_TXOFF		71

#define EDAS_LED_ERRON		80
#define EDAS_LED_ERROFF		81

#define EDAS_LED_KON		90
#define EDAS_LED_KOFF		91

#define EDAS_WD_ON			100
#define EDAS_WD_OFF			101


#define edas_power_on()		ioctl(g_nGpioFd, EDAS_POWERON)
#define edas_power_off()	ioctl(g_nGpioFd, EDAS_POWEROFF)

#define edas_3g_work()		ioctl(g_nGpioFd, EDAS_3G_WORK)
#define edas_3g_off()		ioctl(g_nGpioFd, EDAS_3G_OFF)

#define edas_3g_start()		ioctl(g_nGpioFd, EDAS_3G_START)
#define edas_3g_stop()		ioctl(g_nGpioFd, EDAS_3G_STOP)

#define edas_3g_restarthi()	ioctl(g_nGpioFd, EDAS_3G_RESTARTHI)
#define edas_3g_restartlo()	ioctl(g_nGpioFd, EDAS_3G_RESTARTLO)

#define edas_led_usbon()	ioctl(g_nGpioFd, EDAS_LED_USBON)
#define edas_led_usboff()	ioctl(g_nGpioFd, EDAS_LED_USBOFF)

#define edas_led_rxon()		ioctl(g_nGpioFd, EDAS_LED_RXON)
#define edas_led_rxoff()	ioctl(g_nGpioFd, EDAS_LED_RXOFF)

#define edas_led_txon()		ioctl(g_nGpioFd, EDAS_LED_TXON)
#define edas_led_txoff()	ioctl(g_nGpioFd, EDAS_LED_TXOFF)

#define edas_led_erron()	ioctl(g_nGpioFd, EDAS_LED_ERRON)
#define edas_led_erroff()	ioctl(g_nGpioFd, EDAS_LED_ERROFF)

#define edas_led_kon()		ioctl(g_nGpioFd, EDAS_LED_KON)
#define edas_led_koff()		ioctl(g_nGpioFd, EDAS_LED_KOFF)

#define edas_wd_on()		ioctl(g_nGpioFd, EDAS_WD_ON)
#define edas_wd_off()		ioctl(g_nGpioFd, EDAS_WD_OFF)


typedef struct
{
    unsigned char sts;
    unsigned char counter;
}tLED_STATUS;

tLED_STATUS LedStatus[LED_NUM];


boolean  g_bIsT15On = false;
boolean  g_bIsGpioInitOK = false;


int g_nGpioFd = 0;
int g_nAdcFd = 0;
volatile boolean g_bSysStsThreadRun = false;
pthread_t g_nSysStsThreadId = 0;
unsigned int g_dwSysStsCounter = 0;

void ProcQueryT15Status()
{
    int nT15AdcVal = 0;
    int nT15MilliVolt = 0;
	
	ioctl(g_nAdcFd, 20, &nT15AdcVal);

    nT15MilliVolt = nT15AdcVal * 11;

    if(nT15MilliVolt > 9000)
    {
        g_bIsT15On = true;
    }
    else if(nT15MilliVolt < 3000)
    {
        g_bIsT15On = false;
    }
}

void ProcFeedWatchDog()
{
    static boolean bFeedOn = false;
    if(bFeedOn)
    {
        bFeedOn = false;
        edas_wd_off();
    }
    else
    {
        bFeedOn = true;
        edas_wd_on();
    }		
}

void SetLedOff(unsigned char index)
{
    switch(index)
    {
        case LED_INDEX_USB:
        edas_led_usboff();
        break;
        case LED_INDEX_CANRX:
        edas_led_rxoff();
        break;
        case LED_INDEX_CANTX:
        edas_led_txoff();
        break;
        case LED_INDEX_KLINE:
        edas_led_koff();
        break;
        case LED_INDEX_ERR:
        edas_led_erroff();
        break;
    }
}

void SetLedOn(unsigned char index)
{
    switch(index)
    {
        case LED_INDEX_USB:
        edas_led_usbon();
        break;
        case LED_INDEX_CANRX:
        edas_led_rxon();
        break;
        case LED_INDEX_CANTX:
        edas_led_txon();
        break;
        case LED_INDEX_KLINE:
        edas_led_kon();
        break;
        case LED_INDEX_ERR:
        edas_led_erron();
        break;
    }
}

void SetLedQucikBlink(unsigned char LedIndex,unsigned char BlinkCounter)
{
    if(LedIndex >= LED_NUM)
    {
        return;
    }
    LedStatus[LedIndex].sts = LedStatus[LedIndex].sts&0x0F;
    LedStatus[LedIndex].sts = LedStatus[LedIndex].sts|0x20;
    LedStatus[LedIndex].counter = BlinkCounter;
}

void SetLedSlowBlink(unsigned char LedIndex,unsigned char BlinkCounter)
{
    if(LedIndex >= LED_NUM)
    {
        return;
    }
    LedStatus[LedIndex].sts = LedStatus[LedIndex].sts&0x0F;
    LedStatus[LedIndex].sts = LedStatus[LedIndex].sts|0x10;
    LedStatus[LedIndex].counter = BlinkCounter;
}

void SetLedNormalOff(unsigned char LedIndex)
{
    if(LedIndex >= LED_NUM)
    {
        return;
    }
    LedStatus[LedIndex].sts = 0x00;
    SetLedOff(LedIndex);
}

void SetLedNormalOn(unsigned char LedIndex)
{
    if(LedIndex >= LED_NUM)
    {
        return;
    }
    LedStatus[LedIndex].sts = 0x00;
    SetLedOn(LedIndex);
}

void SetLedNormalAllOff()
{
    unsigned char i;
    for(i=0;i<LED_NUM;i++)
    {
        SetLedNormalOff(i);
    }
}

void SetLedNormalAllOn()
{
    unsigned char i;
    for(i=0;i<LED_NUM;i++)
    {
        SetLedNormalOn(i);
    }
}

void ProcLedBlink(unsigned char ucStsMsk)
{
    unsigned char i;
    for(i=0;i<LED_NUM;i++)
    {
        if(LedStatus[i].sts&ucStsMsk)
        {
            if(LED_BLINK_KEEP_COUNTER == LedStatus[i].counter)
            {
                if(LedStatus[i].sts&0x01)
                {
                    LedStatus[i].sts = LedStatus[i].sts&0xFE;
                    SetLedOff(i);
                }
                else
                {
                    LedStatus[i].sts = LedStatus[i].sts|0x01;
                    SetLedOn(i);
                }
            }
            else if(0x00 == LedStatus[i].counter)
            {}
            else
            {
                if(LedStatus[i].sts&0x01)
                {
                    LedStatus[i].sts = LedStatus[i].sts&0xFE;
                    LedStatus[i].counter--;
                    SetLedOff(i);
                }
                else
                {
                    LedStatus[i].sts = LedStatus[i].sts|0x01;
                    SetLedOn(i);
                }
            }
        }
    }
}

void ThreadSystemStatusProc()
{
    g_dwSysStsCounter = 0;
    while(g_bSysStsThreadRun)
    {
        usleep(50000);
        g_dwSysStsCounter++;
        if(0 == (g_dwSysStsCounter%6))
        {
            ProcLedBlink(0x10);
        }
        if(0 == (g_dwSysStsCounter%1))
        {
            ProcLedBlink(0x20);
        }
        ProcQueryT15Status();
        ProcFeedWatchDog();
    }
}

boolean InitAdc()
{
    g_nAdcFd = open("/dev/magic-adc", 0);
	if (g_nAdcFd < 0) 
    {
        perror("open /dev/magic-adc");
        printf("GPIO Init failed open /dev/magic-adc \n");
        return false;
	}
    return true;
}

void ExitAdc()
{
    close(g_nAdcFd);
    g_nAdcFd = 0;
}

boolean InitGpio()
{
    int i;
    g_bIsGpioInitOK = false;
    for(i=0;i<LED_NUM;i++)
    {
        LedStatus[i].sts = 0;
        LedStatus[i].counter = 0;
    }   

    if(!InitAdc())
    {
        return false;
    }

    g_nGpioFd = open("/dev/edas_gpio", O_RDWR);
	if (g_nGpioFd < 0) 
    {
		perror("open /dev/imx283_gpio");
        printf("GPIO Init failed open /dev/imx283_gpio \n");
        return false;
	}

    g_bIsT15On = false;	

    g_bSysStsThreadRun = true;
    int result = pthread_create(&g_nSysStsThreadId,NULL,(void *)ThreadSystemStatusProc,NULL);
    if(0 != result)
    {
        g_bSysStsThreadRun = false;
        printf("GPIO Init failed create thread \n");
        return false;
    }

    g_bIsGpioInitOK = true;
    printf("GPIO Init Successful  \n");


    return true;
}

void ExitGpio()
{
    if(g_bSysStsThreadRun)
    {
        g_bSysStsThreadRun = false;
        pthread_join(g_nSysStsThreadId, NULL);
        g_nSysStsThreadId = NULL;
    }

    close(g_nGpioFd);
    g_nGpioFd = 0;

    ExitAdc();
}

boolean GetIsT15On()
{
    return g_bIsT15On;
}

boolean GetIsGpioInitOK()
{
    return g_bIsGpioInitOK;
}

boolean Set3GModelPowerOn()
{
    if (g_bIsGpioInitOK) 
    {
        edas_3g_work();
        sleep(1);
        edas_3g_start();
        sleep(1);
        edas_3g_restarthi();
        sleep(1);
        edas_3g_restartlo();
        sleep(1);
        return true;
    }
    printf("GPIO is not Init \n");
    return false;
}

boolean Set3GModelPowerOff()
{
    if (g_bIsGpioInitOK) 
    {
        edas_3g_stop();
        sleep(1);
        edas_3g_off();
        sleep(1);
        return true;
    }
    printf("GPIO is not Init \n");
    return false; 
}

boolean SetEdasPowerOn()
{
    if (g_bIsGpioInitOK) 
    {
        edas_power_on();
        sleep(1);
        return true;
    }
    printf("GPIO is not Init \n");
    return false; 
}

boolean SetEdasPowerOff()
{
    if (g_bIsGpioInitOK) 
    {
        edas_power_off();
        sleep(1);
        return true;
    }
    printf("GPIO is not Init \n");
    return false; 
}

