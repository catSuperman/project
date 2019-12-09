#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "RawCanMsgLogFile.h"
#include "rtc.h"
#include "DaqConfig.h"
#include "DBC_SignalExplain.h"



#define RECORD_PATH "/media/sd-mmcblk0p1/record/"

unsigned char pBufftoWifi[15]={0};
char g_FileId[64] = {'E','D','A','S','_','P','\0'};
boolean g_bLogFileInitOK = false;
FILE * g_pLogFile = NULL;
tCAN_RAW_MSG_QUEUE g_LogCanRawMsgQueue;
unsigned int g_nCurrLogMsgCount = 0;

pthread_mutex_t g_WritLogFileMutex;
pthread_t g_nCanDisposeThreadId = 0;

char g_CurLogFileName[256] = {0};

tCAN_RAW_MSG msg;
int SignalDisposeFlag = 0;
int startThreadFlag = 1;


boolean GetNewFileName(char* pFileName)
{
    int nRecordPathLength = strlen(RECORD_PATH);
    int nFileIDLength = strlen(g_FileId);
	int nDevIDLength = strlen(g_cDeviceName);

	
    memset(pFileName,0,256);
	memcpy(pFileName,RECORD_PATH,nRecordPathLength);
	memcpy(&pFileName[nRecordPathLength],g_cDeviceName,nDevIDLength);
	pFileName[nRecordPathLength+nDevIDLength] = '_';
	memcpy(&pFileName[nRecordPathLength+nDevIDLength+1],g_FileId,nFileIDLength);

    unsigned char year,month,date,hour,minute,second;
	unsigned char rtctime[8];

	if(!GetRtcTime(rtctime))
    {
        return false;
    }
	printf("rtctime : %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",rtctime[0],rtctime[1],rtctime[2],
	rtctime[3],rtctime[4],rtctime[5],rtctime[6],rtctime[7]);
	
	year   = rtctime[6];
	month  = 0x1F & rtctime[5];
	date   = 0x3F & rtctime[4];
	hour   = 0x3F & rtctime[2];
	minute = 0x7F & rtctime[1];
	second = 0x7F & rtctime[0];

	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+0] = '0'+(year >> 4);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+1] = '0'+(year & 0x0F);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+2] = '0'+(month >> 4);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+3] = '0'+(month & 0x0F);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+4] = '0'+(date >> 4);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+5] = '0'+(date & 0x0F);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+6] = '0'+(hour >> 4);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+7] = '0'+(hour & 0x0F);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+8] = '0'+(minute >> 4);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+9] = '0'+(minute & 0x0F);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+10] = '0'+(second >> 4);
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+11] = '0'+(second & 0x0F);
	
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+12] = '.';
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+13] = 'c';
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+14] = 'r';
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+15] = 'm';
	
	pFileName[nRecordPathLength+nDevIDLength+1+nFileIDLength+16] = 0;

	printf("Get New File : %s \n",pFileName);

	return true;
}

boolean GetSDStoreStatus(unsigned int *pTotalSize,unsigned int *pUsedSize,unsigned int *pFreeSize)
{
	char buf[200] = {0};
	FILE* sd_fp = NULL;
	int read_cnt = 0;
	char *p[10];
	int i;

	printf("Get SD Status: start open mmcblk0p1 \n");
	
	sd_fp = popen("df /media/sd-mmcblk0p1/  | grep /dev/mmcblk0p1 ", "r");
	if ( sd_fp != NULL ) 
    {
        read_cnt = fread(buf, sizeof(char), 200-1, sd_fp);
		
        if (read_cnt > 0) 
        {
			p[0] = strtok(buf," ");
		
			for(i=1;i<6;i++)
			{
				p[i] = strtok(NULL," ");
			}

			*pTotalSize = atoi(p[1]);
			*pUsedSize =  atoi(p[2]);
			*pFreeSize =  atoi(p[3]);
			pclose(sd_fp);
			printf("Get SD Status: Total(%d Byte) Used(%d Byte) Free(%d Byte) \n",*pTotalSize,*pUsedSize,*pFreeSize);
            return true;
        }
        else
        {
			pclose(sd_fp);
			printf("Get SD Status: open mmcblk0p1 empty \n");
			return false;
        }
    }
	else
    {
		printf("Get SD Status: open mmcblk0p1 failed \n");
        return false;
    }

	return false;
}

boolean SetLogInit(char* pFileID)
{
    if(g_bLogFileInitOK)
    {
        return false;
    }

	if(g_pLogFile)
	{
		return false;
	}

	unsigned int nTotalSize = 0;
	unsigned int nUsedSize = 0;
	unsigned int nFreeSize = 0;
	int  results = 0;
	pthread_t g_nCan0WifiThreadId = 0;

	if(!GetSDStoreStatus(&nTotalSize,&nUsedSize,&nFreeSize))
	{
		return false;
	}

    memcpy(g_FileId,pFileID,64);
	g_bLogFileInitOK = true;


	memset(g_CurLogFileName,0,256);
	if(!GetNewFileName(g_CurLogFileName))
	{
		return false;
	}

	pthread_mutex_init(&g_WritLogFileMutex, NULL);
	InitRawMsgQueue(&g_LogCanRawMsgQueue);
	g_nCurrLogMsgCount = 0;

	g_pLogFile = fopen(g_CurLogFileName,"w+b");	
	if(g_pLogFile != NULL)
	{
		printf("Set Log Init: open log file (%s) successful \n",g_CurLogFileName);
		printf("open success\n");
		return true;
	}
	return false;
}

tCAN_MSG_TABLE TableMsg[7];

void DataToTable(unsigned long canid,unsigned char *can_table)
{
    int i;
    for(i=0;i<7;i++)
    {
        TableMsg[i].flag = 0;
    }
        for(i=0;i<7;i++)
        {
            if(TableMsg[i].flag == 0 && canid !=0)
            {
                TableMsg[i].id = canid;
                memcpy(&TableMsg[i].ucData,&can_table,8);
                TableMsg[i].flag = 1;
                break;
            }
            else
            {
                if(TableMsg[i].id == canid)
                {
                    memcpy(&TableMsg[i].ucData,&can_table,8);
                    break;
                }
            }
		}
}


void ConvertCanRawMsgToBuff(tCAN_RAW_MSG* msg,unsigned char* pBuff,unsigned char* pBuffWifi)
{
	pBuff[0] = (unsigned char)(msg->dwMsgID&0x000000FF);
	pBuff[1] = (unsigned char)((msg->dwMsgID&0x0000FF00)>>8);
	pBuff[2] = (unsigned char)((msg->dwMsgID&0x00FF0000)>>16);
	pBuff[3] = (unsigned char)((msg->dwMsgID&0xFF000000)>>24);

	pBuff[4] = msg->bExt;
	pBuff[5] = msg->bRmt;
	pBuff[6] = msg->bTx;

	double value = msg->dbTimestamp;
	double* p = &value;
	unsigned char* c = (unsigned char*)p;


	pBuff[7] = c[0];
	pBuff[8] = c[1];
	pBuff[9] = c[2];
	pBuff[10] = c[3];
	pBuff[11] = c[4];
	pBuff[12] = c[5];
	pBuff[13] = c[6];
	pBuff[14] = c[7];

	pBuff[15] = msg->ucChan;
	pBuff[16] = msg->ucLen;

	pBuff[17] = msg->ucData[0];
	pBuff[18] = msg->ucData[1];
	pBuff[19] = msg->ucData[2];
	pBuff[20] = msg->ucData[3];
	pBuff[21] = msg->ucData[4];
	pBuff[22] = msg->ucData[5];
	pBuff[23] = msg->ucData[6];
	pBuff[24] = msg->ucData[7];
	
	pBuffWifi[0] = 0xFE;
		pBuffWifi[1] = 0xFD;
		pBuffWifi[2] = (unsigned char)(msg->dwMsgID&0x000000FF);
		pBuffWifi[3] = (unsigned char)((msg->dwMsgID&0x0000FF00)>>8);
		pBuffWifi[4] = (unsigned char)((msg->dwMsgID&0x00FF0000)>>16);
		pBuffWifi[5] = (unsigned char)((msg->dwMsgID&0xFF000000)>>24);
		pBuffWifi[6] = msg->ucData[0];
		pBuffWifi[7] = msg->ucData[1];
		pBuffWifi[8] = msg->ucData[2];
		pBuffWifi[9] = msg->ucData[3];
		pBuffWifi[10] = msg->ucData[4];
		pBuffWifi[11] = msg->ucData[5];
		pBuffWifi[12] = msg->ucData[6];
		pBuffWifi[13] = msg->ucData[7];

		int i=0;
		pBuffWifi[14] = 0;
		while(i<14)
		{
			pBuffWifi[14]  ^= pBuffWifi[i]; 
			i++;
		}

    memcpy(&pBufftoWifi[0],&pBuffWifi[0],15);
	printf("can data wifi: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", pBufftoWifi[0], pBufftoWifi[1], pBufftoWifi[2], pBufftoWifi[3], pBufftoWifi[4], pBufftoWifi[5], pBufftoWifi[6], pBufftoWifi[7]
	,pBufftoWifi[8], pBufftoWifi[9],pBufftoWifi[10], pBufftoWifi[11], pBufftoWifi[12], pBufftoWifi[13], pBufftoWifi[14]);
}





//signal dispose pthread
void SignalDisposeThread()
{
	printf("start thread1\n");
	while(1)
	{
		if(SignalDisposeFlag)
		{
		    DBC_Explain(&msg,BO_List);    //信号解析处理函数
		}
	}
}


void SaveLogCanRawMsgDataToFile()
{
	SignalDisposeFlag = 0;      //信号处理标识
	if(startThreadFlag)
	{
		printf("start thread3\n");
		int  result = 0;
		result = pthread_create(&g_nCanDisposeThreadId,NULL,(void *)SignalDisposeThread,NULL);   //信号处理线程
		if(0 != result)
		{
			printf("signal diapose failse\n");
			return 0;
		}	
		startThreadFlag = 0;
	}
	if(g_pLogFile)
	{
		pthread_mutex_lock(&g_WritLogFileMutex);
		unsigned char pBuff[25] = {0};
		unsigned char pBuffWifi[15]= {0};
		boolean bFlushFlag = false;

		while(OutRawMsgQueue(&g_LogCanRawMsgQueue,&msg))
		{
			SignalDisposeFlag = 1;

			g_nCurrLogMsgCount++;
			//DBC_Explain(&msg,BO_List);
			//DataToTable(msg.dwMsgID,msg.ucData);
			ConvertCanRawMsgToBuff(&msg,pBuff,pBuffWifi);
			//sendCmdWifi2(pBufftoWifi);
			fwrite(pBuff,25,1,g_pLogFile);
			bFlushFlag = true;
		}
		if(bFlushFlag)
		{
			fflush(g_pLogFile);
			system("sync");
			sleep(1);
		}
		if(42949672 <= g_nCurrLogMsgCount)// 文件大于1024M
	//	if(7355443 <= g_nCurrLogMsgCount)// 文件大于160M
		{
			fclose(g_pLogFile);
			g_pLogFile = NULL;
			g_nCurrLogMsgCount = 0;

			memset(g_CurLogFileName,0,256);
			if(GetNewFileName(g_CurLogFileName))
			{
				g_pLogFile = fopen(g_CurLogFileName,"w+b");	
				if(g_pLogFile != NULL)
				{
					printf("Set Log Data To File : open new log file (%s) successful \n",g_CurLogFileName);
				}
				else
				{
					printf("Set Log Data To File : open new log file (%s) failed \n",g_CurLogFileName);
				}
			}
		}
		pthread_mutex_unlock(&g_WritLogFileMutex);

		//printf("Save can msg to log file \n");
	}
}

void SetLogRawMsg(tCAN_RAW_MSG* msg)
{
	InRawMsgQueue(&g_LogCanRawMsgQueue,msg);
	if(IsRawMsgQueueFull(&g_LogCanRawMsgQueue))
	{
		SaveLogCanRawMsgDataToFile();
	}
}

void SetLogExit()
{
	if(g_pLogFile)
	{
		fclose(g_pLogFile);
		g_pLogFile = NULL;
	}
	pthread_mutex_destroy(&g_WritLogFileMutex);
	memset(g_CurLogFileName,0,256);
}