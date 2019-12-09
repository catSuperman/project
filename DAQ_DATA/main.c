
#include "can.h"
#include "rtc.h"
#include "systemtimer.h"
#include "RawCanMsgLogFile.h"
#include "DaqConfig.h"
#include "DaqService.h"
#include "gpio.h"
#include "update.h"
#include "RecordFile.h"
#include <stdio.h>
#include "wifi_esp8266.h"
#include "4g_sim7600.h"
#include "BO_Index.h"
#include "DBC_SignalExplain.h"
#include <pthread.h>



boolean bUpdateSystemDateTimeFlag = false;
tCAN_RAW_MSG pMsgBuf[256];
int nRxMsgCnt = 0;

void InitSystem()
{
   // InitWifi();
    InitLTE4g();

    if(!InitDaqConfig())
    {
        printf("System Init : Set Daqconfig Init failed \n");
        SetError(ERR_INDEX_DAQ_CFG_INIT,true);
    }

    if(!SetLogInit(g_CalDaqCfgFile.m_ucFileID))
    {
        printf("System Init : SetLogInit failed \n");
        SetError(ERR_INDEX_MSG_LOG_INIT,true);
    }

   // if(!SignalBaseConfig())
   // {
   //     printf("Signal config init : failed!\n");
   // }

   
 
    SetEdasPowerOn();


    if(!InitDaqService())
    {
        printf("System Init : Daq Service Init failed \n");
        SetError(ERR_INDEX_DAQ_CAL_INIT,true);
    }

   // if(!InitGpsModel())
   // {
     //   printf("System Init : GPS Model Init failed \n");
      //  SetError(ERR_INDEX_GPS_INIT,true);
   // }
}



int InitWifi()
{
    //打开串口2-----wifi模块
    fd_wifi = openSerialWifi("/dev/ttySP2");
    if(-1 == fd_wifi)
    {
        printf("wifi openSerial falied..");
        return;
    }
    //wifi模块初�?�化
    Set_Wifi();
}
int InitLTE4g()
{
    //打开串口2-----4g模块
    fd_4g = openSerial4g("/dev/ttySP2");
    if(-1 == fd_4g)
    {
        printf("4g openSerial falied..");
        return;
    }
    //4g模块初�?�化
    Set_lte();

}

void ThreadCanQueueMsg()
{
   // while(1)
    //{
        DataPackaging();  
   // } 
}

void ProcRecvCanMsg();
void UpdateSystemDateTime(tCAN_RAW_MSG* msg);
void RunUsbDisk();

void main(int argc,char *argv[])
{
    UpdateProgram();

   

    g_bIsLogCalOdtRawMsg = false;//记录所有的XCP/CCP的ODT报文

    if(!InitRtcTime())
    {
        printf("System Init : InitRtcTime failed \n");
        SetError(ERR_INDEX_RTC_INIT,true);
    }

    InitSystemTimer();

    if(!InitGpio())
    {
        printf("System Init : Set GPIO Init failed \n");
        SetError(ERR_INDEX_GPIO_INIT,true);
    }
    SetLedNormalAllOn();

#if 0
    int nPid = fork();
    if(nPid < 0)
    {
        printf("fork ERROR ... \n");
        return;
    }		
	else if(0 == nPid)
    {
        printf("U-Disk MODE start ... \n");
        RunUsbDisk();
        return;	
	}
	else
	{
        int status = 0;
     // waitpid(nPid, &status, 0 );
        sleep(10);
        printf("DAQ MODE START sub PID %d status %d ... \n",nPid,status);
    }
#endif

    SetLedNormalAllOff();
    SetLedQucikBlink(LED_INDEX_USB,LED_BLINK_KEEP_COUNTER);

    g_GlobalErrorCode = 0;
    InitSystem();
    SetLedSlowBlink(LED_INDEX_USB,LED_BLINK_KEEP_COUNTER);
    if(g_GlobalErrorCode)
    {
         SetLedSlowBlink(LED_INDEX_ERR,2);
    } 

    if(DBCinit())    //DBC��������ʼ��
    {
        printf("DBC file init failed!\n");
    }
    InitInterpreter();


    unsigned char ucHeartCounter = 0;
    boolean bMainRunFlag = true;
    int nT15OffCounter = 0;
    int nDaqCalErrorCounter = 0;
    int nDaqSoftwareResetCounter = 0;
    
    #if 1
    while(bMainRunFlag)
    {
        sleep(1);
       // printf("**** DAQ-APP VERSION 2019-09-07 Heart %d T15 Is %d ****\n",ucHeartCounter,GetIsT15On());

        if(g_GlobalErrorCode)
        {
           SetLedSlowBlink(LED_INDEX_ERR,2);
        }

        ProcRecvCanMsg(); 
        
       if(GetIsT15On())
        {
            nT15OffCounter = 0;
        }
        else
        {
            nT15OffCounter++;
        }

        if(nT15OffCounter >= 10)
        {
            SetLedQucikBlink(LED_INDEX_USB,LED_BLINK_KEEP_COUNTER);
            ExitDaqService();
            SetLogExit();
            SetEdasPowerOff();
            bMainRunFlag = false;
        }

        if(IsDaqRecvMsgTimeout())
        {
            SetError(ERR_INDEX_DAQ_CAL_RX_ODT,true);
        }
        else
        {
            SetError(ERR_INDEX_DAQ_CAL_RX_ODT,false);
        }

        if(IsErrorActive(ERR_INDEX_DAQ_CAL_INIT)||(IsErrorActive(ERR_INDEX_DAQ_CAL_RX_ODT)))
        {
            nDaqCalErrorCounter++;
            if(nDaqSoftwareResetCounter >= 5)
            {
                system("reboot");
            }
        }
        else
        {
            nDaqCalErrorCounter = 0;
            nDaqSoftwareResetCounter = 0;
        }

        if(nDaqCalErrorCounter >= 10)
        {
            SetLedQucikBlink(LED_INDEX_ERR,LED_BLINK_KEEP_COUNTER);

            ExitDaqService();   
            nDaqCalErrorCounter = 0;
            SetError(ERR_INDEX_DAQ_CAL_INIT,false);
            if(!InitDaqService())
            {
                printf("System Init : Daq Service Init failed \n");
                SetError(ERR_INDEX_DAQ_CAL_INIT,true);
            }
            SetLedSlowBlink(LED_INDEX_ERR,2);
            nDaqSoftwareResetCounter++;
        }
        ucHeartCounter++;
    }
    #endif
}

void ProcRecvCanMsg()
{
    
    int i;
    if(g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].ucEnable)
    {
        nRxMsgCnt = 0;
        RecvMsg(0,pMsgBuf,&nRxMsgCnt);
        for(i=0;i<nRxMsgCnt;i++)
        {
            UpdateSystemDateTime(&pMsgBuf[i]);
        }
    }

    if(g_CalDaqCfgFile.m_CfgCanChanDaqSer[1].ucEnable)
    {
        nRxMsgCnt = 0;
        RecvMsg(1,pMsgBuf,&nRxMsgCnt);
        for(i=0;i<nRxMsgCnt;i++)
        {
            UpdateSystemDateTime(&pMsgBuf[i]);
        }
    }
}

void UpdateSystemDateTime(tCAN_RAW_MSG* msg)
{
    if(bUpdateSystemDateTimeFlag)
    {
        return;
    }

    if((true == msg->bExt)&&(0x18DF0001)&&(8 == msg->ucLen)&&(0x00 == msg->ucData[6])&&(0x00 == msg->ucData[7]))
    {
        unsigned char localtime[6] = {0};
        unsigned char rtctime[8];

        if(!GetRtcTime(rtctime))
        {
            return;
        }
        printf("cur system datetime %02X-%02X-%02X %02X:%02X:%02X\n",rtctime[6],rtctime[5],rtctime[4],0x3F&rtctime[2],0x7F&rtctime[1],0x7F&rtctime[0]);

        localtime[0] = (msg->ucData[0]>>4)*10 + (msg->ucData[0]&0x0F);
        localtime[1] = (msg->ucData[1]>>4)*10 + (msg->ucData[1]&0x0F);
        localtime[2] = (msg->ucData[2]>>4)*10 + (msg->ucData[2]&0x0F);
        localtime[3] = (msg->ucData[3]>>4)*10 + (msg->ucData[3]&0x0F);
        localtime[4] = (msg->ucData[4]>>4)*10 + (msg->ucData[4]&0x0F);
        localtime[5] = (msg->ucData[5]>>4)*10 + (msg->ucData[5]&0x0F);
        
        if(!SetRtcTime(localtime))
        {
            return;
        }
        SetLedSlowBlink(LED_INDEX_KLINE,5);
        printf("set system datetime %02X-%02X-%02X %02X:%02X:%02X\n",msg->ucData[0],msg->ucData[1],msg->ucData[2],msg->ucData[3],msg->ucData[4],msg->ucData[5]);
        bUpdateSystemDateTimeFlag = true;
        

        if(!GetRtcTime(rtctime))
        {
            return;
        }
        printf("cur system datetime %02X-%02X-%02X %02X:%02X:%02X\n",rtctime[6],rtctime[5],rtctime[4],0x3F&rtctime[2],0x7F&rtctime[1],0x7F&rtctime[0]);
    }
}

void RunUsbDisk()
{
    system("chmod 755 /root/usb_device");
    sleep(1);
    system("mknod /dev/usb_device c 67 1");
    sleep(1);
    system("./root/usb_device /dev/usb_device");
    sleep(8);
}