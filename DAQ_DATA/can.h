#ifndef __CAN_H_
#define __CAN_H_

#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/socket.h>
#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/raw.h>
#include <pthread.h>
#include "DataTypes.h"
#include <netinet/in.h>

typedef struct
{
	unsigned long  dwMsgID;   //message raw ID
  	boolean   bExt; 
	boolean   bRmt;
	boolean   bTx; 
	unsigned char  ucChan;//CAN 通道
	unsigned char  ucData[8]; //数据�?
	unsigned char  ucLen;//有效数据长度
	double dbTimestamp; //时间�? 单位ms
}tCAN_RAW_MSG;

#define RAW_MSG_QUEUE_SIZE  20240

typedef struct
{
  tCAN_RAW_MSG m_buf[RAW_MSG_QUEUE_SIZE];
	volatile int m_nCnt;
	volatile int m_nRd;
	volatile int m_nWt;
	pthread_mutex_t m_mutex;
}tCAN_RAW_MSG_QUEUE;

#define CAL_ODT_CAN_MSG_MAX_NUM  256
#define CAL_ODT_CAN_MSG_BASE_ID  0x18DD0000

typedef struct
{
	tCAN_RAW_MSG msg;
	boolean bActive;
	
}tCAN_RAW_MSG_CAL_ODT_ITEM;

typedef struct
{
	unsigned long dwMsgId;
	boolean bExt;
}tCAN_RAW_MSG_CAL_ODT_MSG_ID;

typedef struct
{
	tCAN_RAW_MSG_CAL_ODT_ITEM   odt[256];
	tCAN_RAW_MSG_CAL_ODT_MSG_ID dwOdtRxMsgIdList[CAL_ODT_CAN_MSG_MAX_NUM];
	unsigned char  dwOdtRxMsgIdCnt;
	pthread_mutex_t mutex;
}tCAN_RAW_MSG_CAL_ODT;

extern tCAN_RAW_MSG_CAL_ODT g_CalOdt[2];
extern volatile boolean g_RecvCalOdtMsgFlag[2];
extern volatile boolean g_RecvCanBusMsgFlag[2];
extern volatile double g_dbLastRecvCalOdtMsgTimestamp[2];
extern boolean  g_bIsLogCalOdtRawMsg;

void InitRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue);
void ExitRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue);
void ResetRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue);
void InRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue,tCAN_RAW_MSG* msg);
boolean OutRawMsgQueue(tCAN_RAW_MSG_QUEUE* Queue,tCAN_RAW_MSG* msg);
boolean IsRawMsgQueueFull(tCAN_RAW_MSG_QUEUE* Queue);

void SetCanChanBaudrate(int nChanIndex,unsigned short wBaudrate);
void SetCanChanEnable(int nChanIndex, boolean bEnable);
void SetCanChanRxFilter(int nChanIndex,int nFilterIndex,unsigned long dwMsgID,boolean bExt);

boolean InitCan();





void ExitCan();



void SendMsg(int nChanIndex,tCAN_RAW_MSG* msg);
boolean RecvMsg(int nChanIndex,tCAN_RAW_MSG* pMsgBuf,int* pRxMsgCnt);



#endif

