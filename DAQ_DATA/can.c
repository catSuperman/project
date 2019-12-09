#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
#include "can.h"
#include "systemtimer.h"
#include "RawCanMsgLogFile.h"
#include "gpio.h"
#include "DaqConfig.h"

typedef struct
{
	boolean bEnable;
	unsigned short wBaudrate;
	struct can_filter RxFilter[10];
	int nRxFilterCount;
}tCAN_CHAN_CONFIG;

tCAN_RAW_MSG_CAL_ODT g_CalOdt[2];
volatile boolean g_RecvCalOdtMsgFlag[2] = {false};
volatile boolean g_RecvCanBusMsgFlag[2] = {false};
volatile double g_dbLastRecvCalOdtMsgTimestamp[2] = {0};
boolean  g_bIsLogCalOdtRawMsg = true;

const char CanBaudrateString[4][10] = {"125000","250000","500000","1000000"};

tCAN_RAW_MSG_QUEUE g_RxCan0CanRawMsgQueue;
tCAN_RAW_MSG_QUEUE g_RxCan1CanRawMsgQueue;
tCAN_RAW_MSG_QUEUE g_TxCan0CanRawMsgQueue;
tCAN_RAW_MSG_QUEUE g_TxCan1CanRawMsgQueue;

volatile boolean g_bRxTxThreadRun = false;
pthread_t g_nCan0RxThreadId = 0;
pthread_t g_nCan0TxThreadId = 0;
pthread_t g_nCan1RxThreadId = 0;
pthread_t g_nCan1TxThreadId = 0;
pthread_t g_nCanLogSaveThreadId = 0;

int g_nCan0Hand = -1; 
int g_nCan1Hand = -1; 
struct sockaddr_can g_Can0SocketAddr;
struct sockaddr_can g_Can1SocketAddr;
struct ifreq g_Can0Ifreq;
struct ifreq g_Can1Ifreq;
tCAN_CHAN_CONFIG g_Can0Cfg;
tCAN_CHAN_CONFIG g_Can1Cfg;

void SetCanChanBaudrate(int nChanIndex,unsigned short wBaudrate)
{
	switch(nChanIndex)
	{
		case 0:
		g_Can0Cfg.wBaudrate = wBaudrate;
		break;
		case 1:
		g_Can1Cfg.wBaudrate = wBaudrate;
		break;
	}
}

void SetCanChanEnable(int nChanIndex, boolean bEnable)
{
	switch(nChanIndex)
	{
		case 0:
		g_Can0Cfg.bEnable = bEnable;
		break;
		case 1:
		g_Can1Cfg.bEnable = bEnable;
		break;
	}
}

void SetCanChanRxFilter(int nChanIndex,int nFilterIndex,unsigned long dwMsgID,boolean bExt)
{
	switch(nChanIndex)
	{
		case 0:
		g_Can0Cfg.nRxFilterCount = nFilterIndex + 1;
		if(g_Can0Cfg.nRxFilterCount > 10)
		{
			return;
		}
		else
		{
			if(dwMsgID == 0xFFFFFFFF)
			{
				g_Can0Cfg.RxFilter[nFilterIndex].can_mask = 0;
			}
			else
			{
				if(bExt)
				{
					g_Can0Cfg.RxFilter[nFilterIndex].can_id   = CAN_EFF_FLAG|dwMsgID;
					g_Can0Cfg.RxFilter[nFilterIndex].can_mask = CAN_EFF_MASK;
				}
				else
				{
					g_Can0Cfg.RxFilter[nFilterIndex].can_id   = dwMsgID;
					g_Can0Cfg.RxFilter[nFilterIndex].can_mask = CAN_SFF_MASK;
				}
			}
		}
		break;
		case 1:
		g_Can1Cfg.nRxFilterCount = nFilterIndex + 1;
		if(g_Can1Cfg.nRxFilterCount > 10)
		{
			return;
		}
		else
		{
			if(0xFFFFFFFF == dwMsgID)
			{
				g_Can0Cfg.RxFilter[nFilterIndex].can_mask = 0;
			}
			else
			{
				if(bExt)
				{
					g_Can1Cfg.RxFilter[nFilterIndex].can_id   = CAN_EFF_FLAG|dwMsgID;
					g_Can1Cfg.RxFilter[nFilterIndex].can_mask = CAN_EFF_MASK;
				}
				else
				{
					g_Can1Cfg.RxFilter[nFilterIndex].can_id   = dwMsgID;
					g_Can1Cfg.RxFilter[nFilterIndex].can_mask = CAN_SFF_MASK;
				}
			}
		}
		break;
	}
}

boolean InitCanDevice_0()
{
	if(0 == g_Can0Cfg.nRxFilterCount)
	{
		printf("CAN0 InitDevice Failed : Rx Filter unset \n");
		return false;
	}

	int  nCanBaudrateStringIndex = -1;
	switch(g_Can0Cfg.wBaudrate)
	{
		case 125:
		nCanBaudrateStringIndex = 0;
		break;
		case 250:
		nCanBaudrateStringIndex = 1;
		break;
		case 500:
		nCanBaudrateStringIndex = 2;
		break;
		case 1000:
		nCanBaudrateStringIndex = 3;
		break;
		default:
		break;
	}
	if(-1 == nCanBaudrateStringIndex)
	{
		printf("CAN0 InitDevice Failed : Unknow Baudrate %d \n",g_Can0Cfg.wBaudrate);
		return false;
	}
	system("ifconfig can0 down");
	
	int fd = -1;
	fd = open("/sys/devices/platform/FlexCAN.0/bitrate",O_CREAT | O_TRUNC | O_WRONLY,0600);
	if(fd < 0)
	{
		printf("CAN0 InitDevice Failed : Open config flie failed \n");
		return false;		
	}
	write(fd,CanBaudrateString[nCanBaudrateStringIndex],strlen(CanBaudrateString[nCanBaudrateStringIndex]));
	close(fd);			
	sleep(1);
	system("ifconfig can0 up");

    int nResult = -1;
	g_nCan0Hand = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	strcpy(g_Can0Ifreq.ifr_name, "can0");
	nResult = ioctl(g_nCan0Hand, SIOCGIFINDEX, &g_Can0Ifreq);
	if (nResult < 0) 
	{
		printf("CAN0 InitDevice Failed : ioctl failed \n");
		return false;
	}
	g_Can0SocketAddr.can_family = PF_CAN;
	g_Can0SocketAddr.can_ifindex = g_Can0Ifreq.ifr_ifindex;
	nResult = bind(g_nCan0Hand, (struct sockaddr *)&g_Can0SocketAddr, sizeof(g_Can0SocketAddr));
	if (nResult < 0) 
	{
		printf("CAN0 InitDevice Failed : bind failed \n");
		return false;
	}
	nResult = setsockopt(g_nCan0Hand, SOL_CAN_RAW, CAN_RAW_FILTER, &g_Can0Cfg.RxFilter[0], g_Can0Cfg.nRxFilterCount * sizeof(g_Can0Cfg.RxFilter[0]));
	if (nResult < 0) 
	{
		printf("CAN0 InitDevice Failed : Set Rx Filter failed \n");
		return false;
	}

	return true;
}

boolean InitCanDevice_1()
{
	if(0 == g_Can1Cfg.nRxFilterCount)
	{
		printf("CAN1 InitDevice Failed : Rx Filter unset \n");
		return false;
	}

	int  nCanBaudrateStringIndex = -1;
	switch(g_Can1Cfg.wBaudrate)
	{
		case 125:
		nCanBaudrateStringIndex = 0;
		break;
		case 250:
		nCanBaudrateStringIndex = 1;
		break;
		case 500:
		nCanBaudrateStringIndex = 2;
		break;
		case 1000:
		nCanBaudrateStringIndex = 3;
		break;
		default:
		break;
	}
	if(-1 == nCanBaudrateStringIndex)
	{
		printf("CAN1 InitDevice Failed : Unknow Baudrate %d \n",g_Can1Cfg.wBaudrate);
		return false;
	}
	system("ifconfig can1 down");
	
	int fd = -1;
	fd = open("/sys/devices/platform/FlexCAN.1/bitrate",O_CREAT | O_TRUNC | O_WRONLY,0600);
	if(fd < 0)
	{
		printf("CAN1 InitDevice Failed : Open config flie failed \n");
		return false;		
	}
	write(fd,CanBaudrateString[nCanBaudrateStringIndex],strlen(CanBaudrateString[nCanBaudrateStringIndex]));
	close(fd);			
	sleep(1);
	system("ifconfig can1 up");


	int nResult = -1;
	g_nCan1Hand = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	strcpy(g_Can1Ifreq.ifr_name, "can1");
	nResult = ioctl(g_nCan1Hand, SIOCGIFINDEX, &g_Can1Ifreq);
	if (nResult < 0) 
	{
		printf("CAN1 InitDevice Failed : ioctl failed \n");
		return false;
	}
	g_Can1SocketAddr.can_family = PF_CAN;
	g_Can1SocketAddr.can_ifindex = g_Can1Ifreq.ifr_ifindex;
	nResult = bind(g_nCan1Hand, (struct sockaddr *)&g_Can1SocketAddr, sizeof(g_Can1SocketAddr));
	if (nResult < 0) 
	{
		printf("CAN1 InitDevice Failed : bind failed \n");
		return false;
	}
	nResult = setsockopt(g_nCan1Hand, SOL_CAN_RAW, CAN_RAW_FILTER, &g_Can1Cfg.RxFilter[0], g_Can1Cfg.nRxFilterCount * sizeof(g_Can1Cfg.RxFilter[0]));
	if (nResult < 0) 
	{
		printf("CAN1 InitDevice Failed : Set Rx Filter failed \n");
		return false;
	}

	return true;
}

void InitRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue)
{
	pthread_mutex_init(&Queue->m_mutex, NULL);
	Queue->m_nCnt = 0;
	Queue->m_nRd = 0;
	Queue->m_nWt = 0;
}

void ExitRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue)
{
	pthread_mutex_destroy(&Queue->m_mutex);
}

void ResetRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue)
{
	pthread_mutex_lock(&Queue->m_mutex);
	Queue->m_nCnt = 0;		Queue->m_nRd = 0;		Queue->m_nWt = 0;
	pthread_mutex_unlock(&Queue->m_mutex);
}

void InRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue,tCAN_RAW_MSG* msg)
{
	pthread_mutex_lock(&Queue->m_mutex);
	memcpy(&Queue->m_buf[Queue->m_nWt],msg,sizeof(Queue->m_buf[Queue->m_nWt]));
	Queue->m_nWt++;
	if (Queue->m_nWt >= RAW_MSG_QUEUE_SIZE)
	{
		Queue->m_nWt = 0;
	}
	Queue->m_nCnt++;
	if (Queue->m_nCnt > RAW_MSG_QUEUE_SIZE)
	{
		Queue->m_nCnt = RAW_MSG_QUEUE_SIZE;
		Queue->m_nRd++;
		if (Queue->m_nRd >= RAW_MSG_QUEUE_SIZE)
		{
			Queue->m_nRd = 0;
		}
	}
	pthread_mutex_unlock(&Queue->m_mutex);
}

boolean OutRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue,tCAN_RAW_MSG* msg)
{
	pthread_mutex_lock(&Queue->m_mutex);
	if (Queue->m_nCnt <= 0)
	{
		pthread_mutex_unlock(&Queue->m_mutex);
		return false;
	}
	memcpy(msg,&Queue->m_buf[Queue->m_nRd],sizeof(Queue->m_buf[Queue->m_nRd]));
	Queue->m_nRd++;
	if (Queue->m_nRd >= RAW_MSG_QUEUE_SIZE)
	{
		Queue->m_nRd = 0;
	}
	Queue->m_nCnt--;
	pthread_mutex_unlock(&Queue->m_mutex);
	return true;
}

boolean IsRawMsgQueueFull(tCAN_RAW_MSG_QUEUE* Queue)
{
	boolean bFull = false;
	pthread_mutex_lock(&Queue->m_mutex);
	if(Queue->m_nCnt >= RAW_MSG_QUEUE_SIZE)
	{
		bFull = true;
	}
	pthread_mutex_unlock(&Queue->m_mutex);
	return bFull;
}

void RecvCalOdtMsg(int nChanIndex,tCAN_RAW_MSG* msg)
{
	if(g_RecvCalOdtMsgFlag[nChanIndex])
	{
		unsigned char i = 0;
		for(i=0;i<g_CalOdt[nChanIndex].dwOdtRxMsgIdCnt;i++)
		{
			if((msg->bExt == g_CalOdt[nChanIndex].dwOdtRxMsgIdList[i].bExt)&&(msg->dwMsgID == g_CalOdt[nChanIndex].dwOdtRxMsgIdList[i].dwMsgId))
			{
				g_dbLastRecvCalOdtMsgTimestamp[nChanIndex] = GetSystemTimeMs();
				if(g_bIsLogCalOdtRawMsg)
				{
				
					g_CalOdt[nChanIndex].odt[msg->ucData[0]].msg.dbTimestamp = msg->dbTimestamp;
					memcpy(g_CalOdt[nChanIndex].odt[msg->ucData[0]].msg.ucData,msg->ucData,8);
					SetLogRawMsg(&g_CalOdt[nChanIndex].odt[msg->ucData[0]].msg);
				}
				else
				{

					pthread_mutex_lock(&g_CalOdt[nChanIndex].mutex);
					g_CalOdt[nChanIndex].odt[msg->ucData[0]].msg.dbTimestamp = msg->dbTimestamp;
					memcpy(g_CalOdt[nChanIndex].odt[msg->ucData[0]].msg.ucData,msg->ucData,8);
					g_CalOdt[nChanIndex].odt[msg->ucData[0]].bActive = true;
					pthread_mutex_unlock(&g_CalOdt[nChanIndex].mutex);
				}
			}
		}
	}
}

void RecvCanBusMsg(int nChanIndex,tCAN_RAW_MSG* msg)
{
	if(g_RecvCanBusMsgFlag[nChanIndex])
	{
		SetLogRawMsg(msg);
	}
}

tCAN_RAW_MSG this_msg;
tCAN_RAW_MSG last_msg;

void ThreadCan0RxCanMsg()
{
	fd_set rset;
	int nResult = -1;
	struct can_frame RxCanFrame;
	tCAN_RAW_MSG msg;
	int i = 0;
	boolean bFind = false;
	while(g_bRxTxThreadRun)
	{
		FD_ZERO(&rset);
		FD_SET(g_nCan0Hand,&rset);
		nResult = read(g_nCan0Hand,&RxCanFrame,sizeof(RxCanFrame));
		if(nResult < sizeof(RxCanFrame))
		{
			continue;
		}
		if(RxCanFrame.can_id & CAN_ERR_FLAG)
		{
			continue;
		}

		if((0 == g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].ucCalDaqType)||(1 == g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].ucCalDaqType))
		{
			bFind = false;
			for(i=0;i<g_Can0Cfg.nRxFilterCount;i++)
			{
				if(RxCanFrame.can_id == g_Can0Cfg.RxFilter[i].can_id)
				{
					bFind = true;
					break;
				}
			}

			if(!bFind)
			{
				continue;
			}
		}

		msg.bTx = false;
		msg.dbTimestamp = GetSystemTimeMs();
		msg.ucChan = 0;
		msg.ucLen = RxCanFrame.can_dlc;
		memcpy(msg.ucData,RxCanFrame.data,8);
		if(RxCanFrame.can_id&CAN_EFF_FLAG)
		{
			msg.bExt = true;
			msg.dwMsgID = RxCanFrame.can_id&CAN_EFF_MASK;
		}
		else
		{
			msg.bExt = false;
			msg.dwMsgID = RxCanFrame.can_id&CAN_SFF_MASK;
		}
		msg.bRmt = false;

		memcpy(&last_msg,&this_msg,sizeof(this_msg));
		memcpy(&this_msg,&msg,     sizeof(this_msg));
		if((this_msg.bExt == last_msg.bExt)&&(this_msg.dwMsgID == last_msg.dwMsgID)&&(this_msg.dbTimestamp == last_msg.dbTimestamp)&&(this_msg.ucChan == last_msg.ucChan))
		{
			if((this_msg.ucData[0] == last_msg.ucData[0])
			&&(this_msg.ucData[1] == last_msg.ucData[1])
			&&(this_msg.ucData[2] == last_msg.ucData[2])
			&&(this_msg.ucData[3] == last_msg.ucData[3])
			&&(this_msg.ucData[4] == last_msg.ucData[4])
			&&(this_msg.ucData[5] == last_msg.ucData[5])
			&&(this_msg.ucData[6] == last_msg.ucData[6])
			&&(this_msg.ucData[7] == last_msg.ucData[7]))
			{
				continue;
			}
		}

		//381    ... 897    ï¿½ï¿½Ñ¹
		//382    ... 898
		//281    ... 641    ï¿½ï¿½ï¿½ï¿½Ù¶ï¿?
		//282    ... 642
		//301    ... 769    ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
		//302    ... 770
		//201    ... 513    ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Â¶ï¿½
		//202    ... 514
       if(msg.dwMsgID == 393   || msg.dwMsgID == 187  ||  msg.dwMsgID == 897  || msg.dwMsgID == 0x180850F4 || msg.dwMsgID == 0x18DF0001
	  || msg.dwMsgID == 0x180750F4  || msg.dwMsgID == 513 )
        {

		InRawMsgQueue(&g_RxCan0CanRawMsgQueue,&msg);
		RecvCalOdtMsg(0,&msg);
		
		 RecvCanBusMsg(0,&msg);
		}
		
		SetLedQucikBlink(LED_INDEX_CANRX,1);
	}	
	g_nCan0RxThreadId = 0;
}

void ThreadCan1RxCanMsg()
{
	fd_set rset;
	int nResult = -1;
	struct can_frame RxCanFrame;
	tCAN_RAW_MSG msg;
	int i = 0;
	boolean bFind = false;
	while(g_bRxTxThreadRun)
	{
		FD_ZERO(&rset);
		FD_SET(g_nCan1Hand,&rset);
		nResult = read(g_nCan1Hand,&RxCanFrame,sizeof(RxCanFrame));

		if(nResult < sizeof(RxCanFrame))
		{
			continue;
		}
		if(RxCanFrame.can_id & CAN_ERR_FLAG)
		{
			continue;
		}

		if((0 == g_CalDaqCfgFile.m_CfgCanChanDaqSer[1].ucCalDaqType)||(1 == g_CalDaqCfgFile.m_CfgCanChanDaqSer[1].ucCalDaqType))
		{
			bFind = false;
			for(i=0;i<g_Can1Cfg.nRxFilterCount;i++)
			{
				if(RxCanFrame.can_id == g_Can1Cfg.RxFilter[i].can_id)
				{
					bFind = true;
					break;
				}
			}

			if(!bFind)
			{
				continue;
			}
		}
		
		msg.bTx = false;
		msg.dbTimestamp = GetSystemTimeMs();
		msg.ucChan = 1;
		msg.ucLen = RxCanFrame.can_dlc;
		memcpy(msg.ucData,RxCanFrame.data,8);
		if(RxCanFrame.can_id&CAN_EFF_FLAG)
		{
			msg.bExt = true;
			msg.dwMsgID = RxCanFrame.can_id&CAN_EFF_MASK;
		}
		else
		{
			msg.bExt = false;
			msg.dwMsgID = RxCanFrame.can_id&CAN_SFF_MASK;
		}
		msg.bRmt = false;
		InRawMsgQueue(&g_RxCan1CanRawMsgQueue,&msg);
		RecvCalOdtMsg(1,&msg);
		RecvCanBusMsg(1,&msg);
		SetLedQucikBlink(LED_INDEX_CANRX,1);
	}	
	g_nCan1RxThreadId = 0;
}

void ThreadCan0TxCanMsg()
{
	struct can_frame TxCanFrame;
	tCAN_RAW_MSG msg;
	while(g_bRxTxThreadRun)
	{
		while(OutRawMsgQueue(&g_TxCan0CanRawMsgQueue,&msg))
		{
			if(msg.bExt)
			{
				TxCanFrame.can_id = msg.dwMsgID|CAN_EFF_FLAG;
			}
			else
			{
				TxCanFrame.can_id = msg.dwMsgID;
			}
			TxCanFrame.can_dlc = msg.ucLen;
			memcpy(TxCanFrame.data,msg.ucData,8);
			write(g_nCan0Hand,&TxCanFrame,sizeof(TxCanFrame));
			msg.dbTimestamp = GetSystemTimeMs();
			SetLedQucikBlink(LED_INDEX_CANTX,1);
		}
	}
	g_nCan0TxThreadId = 0;
	
}

void ThreadCan1TxCanMsg()
{
	struct can_frame TxCanFrame;
	tCAN_RAW_MSG msg;
	while(g_bRxTxThreadRun)
	{
		while(OutRawMsgQueue(&g_TxCan1CanRawMsgQueue,&msg))
		{
			if(msg.bExt)
			{
				TxCanFrame.can_id = msg.dwMsgID|CAN_EFF_FLAG;
			}
			else
			{
				TxCanFrame.can_id = msg.dwMsgID;
			}
			TxCanFrame.can_dlc = msg.ucLen;
			memcpy(TxCanFrame.data,msg.ucData,8);
			write(g_nCan1Hand,&TxCanFrame,sizeof(TxCanFrame));
			msg.dbTimestamp = GetSystemTimeMs();
			SetLedQucikBlink(LED_INDEX_CANTX,1);
		}
	}
	g_nCan1TxThreadId = 0;
}

void ThreadCanLogSaveMsg()
{
	double dbSaveTimestamp = GetSystemTimeMs();
	double dbCurTimestamp  = dbSaveTimestamp;
	while(g_bRxTxThreadRun)
	{
		dbCurTimestamp = GetSystemTimeMs();
		if((dbCurTimestamp - dbSaveTimestamp) >= 200
		)
		{
			dbSaveTimestamp = dbCurTimestamp;
			SaveLogCanRawMsgDataToFile();
		}
		else
		{
			//sleep(1);
		}
	}
	SaveLogCanRawMsgDataToFile();
	g_nCanLogSaveThreadId = NULL;
}

boolean InitCan()
{
	if(g_bRxTxThreadRun)
	{
		printf("CAN Init Failed : CAN has been Init  \n");
		return false;
	}

	InitRawMsgQueue(&g_RxCan0CanRawMsgQueue);
	InitRawMsgQueue(&g_RxCan1CanRawMsgQueue);
	InitRawMsgQueue(&g_TxCan0CanRawMsgQueue);
	InitRawMsgQueue(&g_TxCan1CanRawMsgQueue);

	int i = 0;
    g_CalOdt[0].dwOdtRxMsgIdCnt = 0;
    pthread_mutex_init(&g_CalOdt[0].mutex, NULL);
    for(i=0;i<CAL_ODT_CAN_MSG_MAX_NUM;i++)
    {
        g_CalOdt[0].dwOdtRxMsgIdList[i].dwMsgId = 0xFFFFFFFF;
        g_CalOdt[0].dwOdtRxMsgIdList[i].bExt = 0;
    }
    for(i=0;i<256;i++)
    {
        g_CalOdt[0].odt[i].bActive = false;
		g_CalOdt[0].odt[i].msg.dwMsgID = 0xFFFFFFFF;
        memset(&g_CalOdt[0].odt[i].msg.ucData[0],0,8);
		g_CalOdt[0].odt[i].msg.dbTimestamp = 0;
		g_CalOdt[0].odt[i].msg.bExt = true;
		g_CalOdt[0].odt[i].msg.bRmt = false;
		g_CalOdt[0].odt[i].msg.bTx = false;
		g_CalOdt[0].odt[i].msg.ucLen = 8;
		g_CalOdt[0].odt[i].msg.ucChan = 0;
    }
	g_CalOdt[1].dwOdtRxMsgIdCnt = 0;
    pthread_mutex_init(&g_CalOdt[1].mutex, NULL);
    for(i=0;i<CAL_ODT_CAN_MSG_MAX_NUM;i++)
    {
        g_CalOdt[1].dwOdtRxMsgIdList[i].dwMsgId = 0xFFFFFFFF;
        g_CalOdt[1].dwOdtRxMsgIdList[i].bExt = 0;
    }
    for(i=0;i<256;i++)
    {
        g_CalOdt[1].odt[i].bActive = false;
		g_CalOdt[1].odt[i].msg.dwMsgID = 0xFFFFFFFF;
        memset(&g_CalOdt[1].odt[i].msg.ucData[0],0,8);
		g_CalOdt[1].odt[i].msg.dbTimestamp = 0;
		g_CalOdt[1].odt[i].msg.bExt = true;
		g_CalOdt[1].odt[i].msg.bRmt = false;
		g_CalOdt[1].odt[i].msg.bTx = false;
		g_CalOdt[1].odt[i].msg.ucLen = 8;
		g_CalOdt[1].odt[i].msg.ucChan = 1;
    }
	g_RecvCalOdtMsgFlag[0] = false;
	g_RecvCalOdtMsgFlag[1] = false;
	g_RecvCanBusMsgFlag[0] = false;
	g_RecvCanBusMsgFlag[1] = false;
	g_dbLastRecvCalOdtMsgTimestamp[0] = GetSystemTimeMs();
	g_dbLastRecvCalOdtMsgTimestamp[1] = GetSystemTimeMs();

	int result = 0;
	int result_wifi = 0;

	if(g_Can0Cfg.bEnable)
	{

		if(InitCanDevice_0())
		{
			g_bRxTxThreadRun = true;
			result = pthread_create(&g_nCan0RxThreadId,NULL,(void *)ThreadCan0RxCanMsg,NULL);
			if(0 != result)
			{
				g_bRxTxThreadRun = false;
				return false;
			}

			result = pthread_create(&g_nCan0TxThreadId,NULL,(void *)ThreadCan0TxCanMsg,NULL);
			if(0 != result)
			{
				g_bRxTxThreadRun = false;
				return false;
			}
			printf("CAN0 Init Successful : baudrate %d \n",g_Can0Cfg.wBaudrate);
		}
		else
		{
			return false;
		}
	}

	if(g_Can1Cfg.bEnable)
	{
		if(InitCanDevice_1())
		{
			g_bRxTxThreadRun = true;
			result = pthread_create(&g_nCan1RxThreadId,NULL,(void *)ThreadCan1RxCanMsg,NULL);
			if(0 != result)
			{
				g_bRxTxThreadRun = false;
				return false;
			}

			result = pthread_create(&g_nCan1TxThreadId,NULL,(void *)ThreadCan1TxCanMsg,NULL);
			if(0 != result)
			{
				g_bRxTxThreadRun = false;
				return false;
			}
			printf("CAN1 Init Successful : baudrate %d \n",g_Can1Cfg.wBaudrate);
		}
		else
		{
			return false;
		}
	}

	if(g_bRxTxThreadRun)
	{
		result = pthread_create(&g_nCanLogSaveThreadId,NULL,(void *)ThreadCanLogSaveMsg,NULL);
		if(0 != result)
		{
			g_bRxTxThreadRun = false;
			return false;
		}

	}

	return true;
}

void ExitCan()
{
	if(g_bRxTxThreadRun)
	{
		g_bRxTxThreadRun = false;
		sleep(2);
	
		system("ifconfig can0 down");
		system("ifconfig can1 down");

		printf("CAN Exit Successful \n");
	}

	pthread_mutex_destroy(&g_CalOdt[0].mutex);
    pthread_mutex_destroy(&g_CalOdt[1].mutex);
}

void SendMsg(int nChanIndex,tCAN_RAW_MSG* msg)
{
	switch(nChanIndex)
	{
		case 0:
		msg->ucChan = 0;
		InRawMsgQueue(&g_TxCan0CanRawMsgQueue,msg);
		break;
		case 1:
		msg->ucChan = 1;
		InRawMsgQueue(&g_TxCan1CanRawMsgQueue,msg);
		break;
	}
}

boolean RecvMsg(int nChanIndex,tCAN_RAW_MSG* pMsgBuf,int* pRxMsgCnt)
{
	*pRxMsgCnt = 0;
	switch(nChanIndex)
	{
		case 0:
		while(OutRawMsgQueue(&g_RxCan0CanRawMsgQueue,&pMsgBuf[*pRxMsgCnt]))
		{
			*pRxMsgCnt = *pRxMsgCnt + 1;
			if((*pRxMsgCnt) >= 256)
			{
				return true;
			}
		}
		break;
		case 1:
		while(OutRawMsgQueue(&g_RxCan1CanRawMsgQueue,&pMsgBuf[*pRxMsgCnt]))
		{
			*pRxMsgCnt = *pRxMsgCnt + 1;
			if((*pRxMsgCnt) >= 256)
			{
				return true;
			}
		}
		break;
	}
	if(0 == (*pRxMsgCnt))
	{
		return false;
	}
	return true;
}












