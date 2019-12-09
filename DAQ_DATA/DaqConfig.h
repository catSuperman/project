#ifndef __DAQ_CONFIG_H_
#define __DAQ_CONFIG_H_

#include "DataTypes.h"


typedef struct
{
	unsigned char ucCanChanIndex;
	unsigned char ucPID;
	unsigned char ucDataOffset;
	unsigned int  dwAddress;
	unsigned char ucDataSize;
	char   ucName[64];
}tDAQ_MNT_SIGNAL;

typedef struct 
{
	unsigned char ucCycTimeChanNum;//0 - 200ms 1-10ms 2-100ms
	unsigned char bIsEnable;
	unsigned int  dwDaqRxMsgID;
}tDAQ_LIST_CHANNEL_CONFIG;

typedef struct 
{
	unsigned char ucCanChanIndex;
	unsigned char ucEnable;
	unsigned short  wBaudrate;
	unsigned char ucCalDaqType;
	unsigned char ucIsByteIntel;
	char  ucDaqObjectName[64];
	unsigned int dwCmdTxMsgID;
	unsigned int dwCmdRxMsgID;
	tDAQ_LIST_CHANNEL_CONFIG DaqListChan[5];
    tDAQ_MNT_SIGNAL MntSignalList[512];
    int nMntSigCount;
}tDAQ_CAN_CHAN_SERVICE_CONFIG;

typedef struct 
{
	unsigned char m_ucCfgFileType;
	unsigned char m_ucIsFileByteIntel;
	unsigned int m_dwReleaseDate;
	unsigned int m_dwReleaseTime;
	char  m_ucFileID[64];
	tDAQ_CAN_CHAN_SERVICE_CONFIG m_CfgCanChanDaqSer[2];
}tCAL_DAQ_CFG_FILE;


extern tCAL_DAQ_CFG_FILE g_CalDaqCfgFile;
extern int  g_nRemoteServerPort;
extern char g_cRemoteServerIP[20];
extern char g_cDeviceName[64];
extern char g_cDeviceAuthKey[64];



boolean IsDaqConfigInitOK();
boolean InitDaqConfig();

void ReadDWord(unsigned char* pData,unsigned int* dwValue,unsigned char bIsByteFmtIntel);
void ReadWord(unsigned char* pData,unsigned short* wValue,unsigned char bIsByteFmtIntel);
void WriteDWord(unsigned char* pData,unsigned int dwValue,unsigned char bIsByteFmtIntel);
void WriteWord(unsigned char* pData,unsigned short wValue,unsigned char bIsByteFmtIntel);
void WriteDouble(unsigned char* pData,double dbValue,unsigned char bIsByteFmtIntel);
void WriteFloat(unsigned char* pData,float fValue,unsigned char bIsByteFmtIntel);




#endif