#ifndef __RAW_CAN_MSG_LOG_FILE_H_
#define __RAW_CAN_MSG_LOG_FILE_H_

#include "can.h"

//extern unsigned char pBufftoWifi[15];

typedef struct
{
	long id;
    unsigned char ucData[8];
    int flag;
}tCAN_MSG_TABLE;

extern char g_CurLogFileName[256];
void SaveLogCanRawMsgDataToFile();
boolean SetLogInit(char* pFileID);
void SetLogRawMsg(tCAN_RAW_MSG* msg);
void SetLogExit();
void SendCanMsgToWifi();



#endif