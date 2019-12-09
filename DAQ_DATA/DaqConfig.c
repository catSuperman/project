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
#include "DaqConfig.h"


#define DAQ_CONFIG_FILE_PATH        "/media/sd-mmcblk0p1/"
#define DAQ_CONFIG_BASE_FILE        "/media/sd-mmcblk0p1/BaseConfig.txt"

#define BLOCK_TYPE_FILE  0x00
#define BLOCK_TYPE_CHAN  0x01
#define BLOCK_TYPE_MNT   0x02
#define BLOCK_TYPE_DAQ   0x03

boolean g_bDaqConfigInitOK = false;
tCAL_DAQ_CFG_FILE g_CalDaqCfgFile;
int  g_nRemoteServerPort = 0;
char g_cRemoteServerIP[20] = {0};
char g_cDeviceName[64] = {0};
char g_cDeviceAuthKey[64] = {0};
char g_DaqConfigFileName[64] = {0};


void ReadDWord(unsigned char* pData,unsigned int* dwValue,unsigned char bIsByteFmtIntel)
{
	if(bIsByteFmtIntel)
	{
		*dwValue = pData[0] + pData[1]*256 + pData[2]*256*256 + pData[3]*256*256*256;
	}
	else
	{
		*dwValue = pData[3] + pData[2]*256 + pData[1]*256*256 + pData[0]*256*256*256;
	}
}

void ReadWord(unsigned char* pData,unsigned short* wValue,unsigned char bIsByteFmtIntel)
{
	if(bIsByteFmtIntel)
	{
		*wValue = pData[0] + pData[1]*256;
	}
	else
	{
		*wValue = pData[1] + pData[0]*256;
	}
}

void WriteDWord(unsigned char* pData,unsigned int dwValue,unsigned char bIsByteFmtIntel)
{
	if(bIsByteFmtIntel)
	{
		pData[0] = (unsigned char)(dwValue&0x000000FF);
		pData[1] = (unsigned char)((dwValue&0x0000FF00)>>8);
		pData[2] = (unsigned char)((dwValue&0x00FF0000)>>16);
		pData[3] = (unsigned char)((dwValue&0xFF000000)>>24);
	}
	else
	{
		pData[3] = (unsigned char)(dwValue&0x000000FF);
		pData[2] = (unsigned char)((dwValue&0x0000FF00)>>8);
		pData[1] = (unsigned char)((dwValue&0x00FF0000)>>16);
		pData[0] = (unsigned char)((dwValue&0xFF000000)>>24);
	}
}

void WriteWord(unsigned char* pData,unsigned short wValue,unsigned char bIsByteFmtIntel)
{
	if(bIsByteFmtIntel)
	{
		pData[0] = (unsigned char)(wValue&0x00FF);
		pData[1] = (unsigned char)((wValue&0xFF00)>>8);
	}
	else
	{
		pData[1] = (unsigned char)(wValue&0x00FF);
		pData[0] = (unsigned char)((wValue&0xFF00)>>8);
	}
}

void WriteDouble(unsigned char* pData,double dbValue,unsigned char bIsByteFmtIntel)
{
	double value = dbValue;
	double* p = &value;
	unsigned char* c = (unsigned char*)p;

	if(bIsByteFmtIntel)
	{
		pData[0] = c[0];
		pData[1] = c[1];
		pData[2] = c[2];
		pData[3] = c[3];
		pData[4] = c[4];
		pData[5] = c[5];
		pData[6] = c[6];
		pData[7] = c[7];
	}
	else
	{
		pData[7] = c[0];
		pData[6] = c[1];
		pData[5] = c[2];
		pData[4] = c[3];
		pData[3] = c[4];
		pData[2] = c[5];
		pData[1] = c[6];
		pData[0] = c[7];
	}
}

void WriteFloat(unsigned char* pData,float fValue,unsigned char bIsByteFmtIntel)
{
	float value = fValue;
	float* p = &value;
	unsigned char* c = (unsigned char*)p;

	if(bIsByteFmtIntel)
	{
		pData[0] = c[0];
		pData[1] = c[1];
		pData[2] = c[2];
		pData[3] = c[3];
	}
	else
	{
		pData[3] = c[0];
		pData[2] = c[1];
		pData[1] = c[2];
		pData[0] = c[3];
	}
}

boolean LoadBaseConfig()
{
    memset(g_DaqConfigFileName,0,64);

    char temp[1024] = {0};
    FILE *fp = NULL;
    int iCount = 0;
    int i = 0;
    char ch = 0;
	fp = fopen(DAQ_CONFIG_BASE_FILE,"rb");
	if(fp == NULL)
	{
		return false;
	}
	
	fseek(fp,0,SEEK_END);
	iCount = ftell(fp);
	rewind(fp);

	boolean bKeyStartFlag = true;
    int nKeyIndex = 0;
    int nKeyReadIndex = 0;
	for(i=0;i<iCount;i++)
	{
		ch = fgetc(fp);
        if(0x7B == ch)//{
        {
            bKeyStartFlag = true;
            nKeyReadIndex = 0;
            memset(temp,0,1024);
        }
        else if(0x7D == ch)//}
        {
            bKeyStartFlag = false;
            switch(nKeyIndex)
            {
                case 0:
                memset(g_DaqConfigFileName,0,64);
                memcpy(g_DaqConfigFileName,temp,nKeyReadIndex);
                break;
                case 1:
                memset(g_cRemoteServerIP,0,20);
                memcpy(g_cRemoteServerIP,temp,nKeyReadIndex);
                case 2:
                g_nRemoteServerPort = atoi(temp);
                break;
                case 3:
                memset(g_cDeviceName,0,64);
                memcpy(g_cDeviceName,temp,nKeyReadIndex);
                break;
                case 4:
                memset(g_cDeviceAuthKey,0,64);
                memcpy(g_cDeviceAuthKey,temp,nKeyReadIndex);
                break;
            }
            nKeyIndex++;
        }
        else
        {
            if(bKeyStartFlag)
            {
                temp[nKeyReadIndex] = ch;
                nKeyReadIndex++;
                if(nKeyReadIndex >= 1024)
                {
                    printf("Load Base config File key length >= 1024 bytes \r\n");
                    return false;
                }
            }
        }
    }
    if(nKeyIndex < 3)
    {
        printf("Load Base config File key count [%d] < 3 \r\n",nKeyIndex);
        return false;
    }
    return true;
}

unsigned char HexStrToVal(char ch)
{
    switch(ch)
    {
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'A':
    case 'a':
        return 10;
    case 'B':
    case 'b':
        return 11;
    case 'C':
    case 'c':
        return 12;
    case 'D':
    case 'd':
        return 13;
    case 'E':
    case 'e':
        return 14;
    case 'F':
    case 'f':
        return 15;
    default:
        break;;
    }
 
    return 0;
}

boolean ReadDaqMntSignal(tDAQ_MNT_SIGNAL* pDaqMntSignal,unsigned char* pData,int* nCurIndex,unsigned char bIsByteFmtIntel)
{
    int nOffset = 0;
    if(0x5A != pData[nOffset])
    {
        printf("ERROR 0x5A != pData[nOffset] %d \r\n",*nCurIndex + nOffset);
        return false;
    }
    nOffset++;

    if(BLOCK_TYPE_MNT != pData[nOffset])
    {
        printf("ERROR BLOCK_TYPE_MNT != pData[nOffset] %d \r\n",*nCurIndex + nOffset);
        return false;
    }
    nOffset++;

    unsigned char ucDataSize = pData[nOffset];
    nOffset++;

    pDaqMntSignal->ucCanChanIndex = pData[nOffset];
    nOffset++;

    pDaqMntSignal->ucPID = pData[nOffset];
    nOffset++;

    pDaqMntSignal->ucDataOffset = pData[nOffset];
    nOffset++;

    ReadDWord(&pData[nOffset],&(pDaqMntSignal->dwAddress),bIsByteFmtIntel);
    nOffset = nOffset + 4;

    pDaqMntSignal->ucDataSize = pData[nOffset];
    nOffset++;

    memcpy(pDaqMntSignal->ucName,&pData[nOffset],64);
    nOffset = nOffset + 64;

    if(nOffset >= 256)
    {
        printf("ERROR nOffset >= 256 %d \r\n",*nCurIndex + nOffset);
        return false;
    }
    unsigned char ucChecksum = 0;
    int i= 0;
    for(i=0;i<nOffset;i++)
    {
        ucChecksum = ucChecksum + pData[i];
    }
    if(ucChecksum != pData[nOffset])
    {
        printf("ucChecksum != pData[nOffset] %d \r\n",*nCurIndex + nOffset);
        return false;
    }
    nOffset++;
    if(ucDataSize != nOffset)
    {
        printf("ucDataSize != nOffset %d \r\n",*nCurIndex + nOffset);
        return false;
    }

    *nCurIndex = *nCurIndex + nOffset;

    printf("ReadDaqMntSignal successful %s 0x%08X %d %d %d \r\n",pDaqMntSignal->ucName,pDaqMntSignal->dwAddress,pDaqMntSignal->ucPID,pDaqMntSignal->ucDataOffset,pDaqMntSignal->ucDataSize);
    return true;
}

boolean ReadDaqListChannelConfig(tDAQ_LIST_CHANNEL_CONFIG* pDaqListChannelConfig,unsigned char* pData,int* nCurIndex,unsigned char bIsByteFmtIntel)
{
    int nOffset = 0;

    if(0x5A != pData[nOffset])
    {
        return false;
    }
    nOffset++;

    if(BLOCK_TYPE_DAQ != pData[nOffset])
    {
        return false;
    }
    nOffset++;

    unsigned char ucDataSize = pData[nOffset];
    nOffset++;

    pDaqListChannelConfig->ucCycTimeChanNum = pData[nOffset];
    nOffset++;

    pDaqListChannelConfig->bIsEnable = pData[nOffset];
    nOffset++;

    ReadDWord(&pData[nOffset],&(pDaqListChannelConfig->dwDaqRxMsgID),bIsByteFmtIntel);
    nOffset = nOffset + 4;

    if(nOffset >= 256)
    {
        return false;
    }
    unsigned char ucChecksum = 0;
    int i = 0;
    for(i=0;i<nOffset;i++)
    {
        ucChecksum = ucChecksum + pData[i];
    }
    if(ucChecksum != pData[nOffset])
    {
        return false;
    }
    nOffset++;
    if(ucDataSize != nOffset)
    {
        return false;
    }

    *nCurIndex = *nCurIndex + nOffset;
    printf("ReadDaqListChannelConfig successful %d 0x%08X %d \r\n",pDaqListChannelConfig->bIsEnable,pDaqListChannelConfig->dwDaqRxMsgID,pDaqListChannelConfig->ucCycTimeChanNum);
    return true;
}

boolean ReadDaqListChanServiceConfig(tDAQ_CAN_CHAN_SERVICE_CONFIG* pDaqListChanServiceConfig,unsigned char* pData,int* nCurIndex,unsigned char bIsByteFmtIntel)
{
    int nOffset = 0;
    if(0x5A != pData[nOffset])
    {
        return false;
    }
    nOffset++;

    if(BLOCK_TYPE_CHAN != pData[nOffset])
    {
        return false;
    }
    nOffset++;

    unsigned char ucDataSize = pData[nOffset];
    nOffset++;

    pDaqListChanServiceConfig->ucCanChanIndex = pData[nOffset];
    nOffset++;

    pDaqListChanServiceConfig->ucEnable = pData[nOffset];
    nOffset++;

    ReadWord(&pData[nOffset],&(pDaqListChanServiceConfig->wBaudrate),bIsByteFmtIntel);
    nOffset = nOffset + 2;

    pDaqListChanServiceConfig->ucCalDaqType = pData[nOffset];
    nOffset++;

    pDaqListChanServiceConfig->ucIsByteIntel = pData[nOffset];
    nOffset++;

    memcpy(pDaqListChanServiceConfig->ucDaqObjectName,&pData[nOffset],64);
    nOffset = nOffset + 64;

    ReadDWord(&pData[nOffset],&(pDaqListChanServiceConfig->dwCmdTxMsgID),bIsByteFmtIntel);
    nOffset = nOffset + 4;

    int cnt = 0;
    int i =0;

    ReadDWord(&pData[nOffset],&(pDaqListChanServiceConfig->dwCmdRxMsgID),bIsByteFmtIntel);
    nOffset = nOffset + 4;
    
    if(nOffset >= 256)
    {
        return false;
    }
    unsigned char ucChecksum = 0;
    for(i=0;i<nOffset;i++)
    {
        ucChecksum = ucChecksum + pData[i];
    }
    if(ucChecksum != pData[nOffset])
    {
        return false;
    }
    nOffset++;
            
    if(ucDataSize != nOffset)
    {
        return false;
    }
    pData[2] = (unsigned char)(nOffset&0x000000FF);


    for(i=0;i<5;i++)
    {
        if(!ReadDaqListChannelConfig(&(pDaqListChanServiceConfig->DaqListChan[i]),&pData[nOffset],&nOffset,bIsByteFmtIntel))
        {
            return false;
        }
    }

    pDaqListChanServiceConfig->nMntSigCount = 0;
    while(ReadDaqMntSignal(&(pDaqListChanServiceConfig->MntSignalList[pDaqListChanServiceConfig->nMntSigCount]),&pData[nOffset],&nOffset,bIsByteFmtIntel))
    {
        pDaqListChanServiceConfig->nMntSigCount++;
    }
    
    *nCurIndex = *nCurIndex + nOffset;

    printf("ReadDaqListChanServiceConfig successful ucCanChanIndex %d  \r\n",pDaqListChanServiceConfig->ucCanChanIndex);
    printf("ReadDaqListChanServiceConfig successful ucEnable %d  \r\n",pDaqListChanServiceConfig->ucEnable);
    printf("ReadDaqListChanServiceConfig successful wBaudrate %d  \r\n",pDaqListChanServiceConfig->wBaudrate);
    printf("ReadDaqListChanServiceConfig successful ucCalDaqType %d  \r\n",pDaqListChanServiceConfig->ucCalDaqType);
    printf("ReadDaqListChanServiceConfig successful ucDaqObjectName %s  \r\n",pDaqListChanServiceConfig->ucDaqObjectName);
    printf("ReadDaqListChanServiceConfig successful dwCmdTxMsgID 0x%08X  \r\n",pDaqListChanServiceConfig->dwCmdTxMsgID);
    return true;
}

boolean ReadCalDaqCfgFile(tCAL_DAQ_CFG_FILE* pCalDaqCfgFile,unsigned char* pData,int* nCurIndex)
{
    int nOffset = 0;

	if(0x5A != pData[nOffset])
	{
        printf("ReadCalDaqCfgFile failed 0x5A != pData[nOffset] \r\n");
		return false;
	}
	nOffset++;

	if(BLOCK_TYPE_FILE != pData[nOffset])
	{
        printf("ReadCalDaqCfgFile failed BLOCK_TYPE_FILE != pData[nOffset] \r\n");
		return false;
	}
	nOffset++;

	unsigned char ucDataSize = pData[nOffset];
	nOffset++;

	pCalDaqCfgFile->m_ucCfgFileType = pData[nOffset];
	nOffset++;

	pCalDaqCfgFile->m_ucIsFileByteIntel = pData[nOffset];
	nOffset++;

	ReadDWord(&pData[nOffset],&(pCalDaqCfgFile->m_dwReleaseDate),pCalDaqCfgFile->m_ucIsFileByteIntel);
	nOffset = nOffset + 4;

	ReadDWord(&pData[nOffset],&(pCalDaqCfgFile->m_dwReleaseTime),pCalDaqCfgFile->m_ucIsFileByteIntel);
	nOffset = nOffset + 4;

	memcpy(pCalDaqCfgFile->m_ucFileID,&pData[nOffset],64);
	nOffset = nOffset + 64;

	if(nOffset >= 256)
	{
		return false;
	}
	
	unsigned char ucChecksum = 0;
    int i = 0;
	for(i=0;i<nOffset;i++)
	{
		ucChecksum = ucChecksum + pData[i];
	}
	if(ucChecksum != pData[nOffset])
	{
        printf("ReadCalDaqCfgFile failed ucChecksum(%d) != pData[nOffset](%d) %d\r\n",ucChecksum,pData[nOffset]);
		return false;
	}
	nOffset++;
	if(ucDataSize != nOffset)
	{
        printf("ReadCalDaqCfgFile failed ucDataSize != nOffset \r\n");
		return false;
	}

	if(!ReadDaqListChanServiceConfig(&(pCalDaqCfgFile->m_CfgCanChanDaqSer[0]),&pData[nOffset],&nOffset,pCalDaqCfgFile->m_ucIsFileByteIntel))
	{
        printf("ReadCalDaqCfgFile failed ReadDaqListChanServiceConfig 0 \r\n");
		return false;
	}
	if(!ReadDaqListChanServiceConfig(&(pCalDaqCfgFile->m_CfgCanChanDaqSer[1]),&pData[nOffset],&nOffset,pCalDaqCfgFile->m_ucIsFileByteIntel))
	{
        printf("ReadCalDaqCfgFile failed ReadDaqListChanServiceConfig 1 \r\n");
		return false;
	}
	*nCurIndex = *nCurIndex + nOffset;
    printf("ReadCalDaqCfgFile successful m_ucCfgFileType %d  \r\n",pCalDaqCfgFile->m_ucCfgFileType);
    printf("ReadCalDaqCfgFile successful m_ucIsFileByteIntel %d  \r\n",pCalDaqCfgFile->m_ucIsFileByteIntel);
    printf("ReadCalDaqCfgFile successful m_dwReleaseDate 0x%08X  \r\n",pCalDaqCfgFile->m_dwReleaseDate);
    printf("ReadCalDaqCfgFile successful m_dwReleaseTime 0x%08X  \r\n",pCalDaqCfgFile->m_dwReleaseTime);
    printf("ReadCalDaqCfgFile successful m_ucFileID %s \r\n",pCalDaqCfgFile->m_ucFileID);
	return true;
}

boolean InitDaqConfig()
{
     g_bDaqConfigInitOK = false;

    memcpy(g_CalDaqCfgFile.m_ucFileID,"HC",3);
    printf("ucFileID %s: \n",g_CalDaqCfgFile.m_ucFileID);
    g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].ucEnable = true;
    g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].wBaudrate = 250;
    g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].dwCmdRxMsgID = 0xFFFFFFFF;
    g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].dwCmdTxMsgID = 0xFFFFFFFF;
     g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].ucCalDaqType = 2;
     g_CalDaqCfgFile.m_dwReleaseTime = 4564646;
    memcpy(g_cDeviceName,"HC01",5);

    g_bDaqConfigInitOK = true;

    printf("config success\n");

    return false;
}

boolean IsDaqConfigInitOK()
{
    return g_bDaqConfigInitOK;
}
#if 0
boolean InitDaqConfig()
{
    g_bDaqConfigInitOK = false;

    if(!LoadBaseConfig())
    {
        printf("Load Base config File failed");
        return false;
    }

    printf("Load Base config File successful %s \r\n",g_DaqConfigFileName);

    char pConfigFileName[256] = {0};
    int nConfigPathLength = strlen(DAQ_CONFIG_FILE_PATH);

    memcpy(pConfigFileName,DAQ_CONFIG_FILE_PATH,nConfigPathLength);
    memcpy(&pConfigFileName[nConfigPathLength],g_DaqConfigFileName,64);

    FILE *fp = NULL;
    int iCount = 0;
    int i = 0;
    char ch = 0;
	fp = fopen(pConfigFileName,"rb");
	if(fp == NULL)
	{
        printf("Load config File failed %s ",pConfigFileName);
		return false;
	}
	
	fseek(fp,0,SEEK_END);
	iCount = ftell(fp);
	rewind(fp);

    unsigned char ucDaqCfgData[204800] = {0};

	boolean bDataStartFlag = false;
    boolean bReadFlag = false;
    int nReadIndex = 0;
    int nDataIndex = 0;
	for(i=0;i<iCount;i++)
	{
		ch = fgetc(fp);
        if(bDataStartFlag)
        {
            if(0x22 == ch)
            {
           //    printf("\r\n %d \r\n",nDataIndex);
                fclose(fp);
                int nCount = 0;
                g_bDaqConfigInitOK = ReadCalDaqCfgFile(&g_CalDaqCfgFile,ucDaqCfgData,&nCount);
                return g_bDaqConfigInitOK;
            }
            if(nReadIndex%2)
            {
                ucDaqCfgData[nDataIndex] = ucDaqCfgData[nDataIndex] + HexStrToVal(ch);
          //      printf("%02X ",ucDaqCfgData[nDataIndex]);
                nDataIndex++;
            }
            else
            {
                ucDaqCfgData[nDataIndex] = HexStrToVal(ch);
                ucDaqCfgData[nDataIndex] = ucDaqCfgData[nDataIndex] << 4;
            }
            
            nReadIndex++;
        }
        else
        {
            if(bReadFlag)
            {
                if('A' == ch)
                {
                    bDataStartFlag = true;
                    ucDaqCfgData[0] = 0x5A;
                    nReadIndex = 2;
                    nDataIndex = 1;
                }
                else
                {
                    bReadFlag = false;                    
                }
            }
            else
            {
                if('5' == ch)
                {
                    bReadFlag = true;
                }
            }
        }
    }
    fclose(fp);
    return false;
}
#endif

