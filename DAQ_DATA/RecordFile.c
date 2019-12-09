
#include <sys/types.h>    
#include <dirent.h>    
#include <stdio.h>    
#include <errno.h> 
#include "RecordFile.h"
#include "RawCanMsgLogFile.h"

#define UP_RECORD_PATH "/media/sd-mmcblk0p1/record/"
#define UP_RECORD_MIRROR_FILE        "/media/sd-mmcblk0p1/UpRecordMirror.txt"



char g_CurTxUpRecordFileName[256] = {0};
unsigned int   g_dwCurTxUpRecordFileSize = 0;
unsigned int   g_dwCurTxUpRecordFileChecksum = 0;
unsigned int   g_dwCurTxUpRecordBlockSize = 0;
unsigned int   g_dwCurTxUpRecordBlockIndex = 0;

 FILE* g_CurOpenUpRecordFile = NULL;



boolean EnumRecordFile(char* pFileName,char* pFilePathName)
{
    DIR *dp;    
    struct dirent *dirp; 
    int n=0; 

    if((dp=opendir(UP_RECORD_PATH))==NULL) 
    {
        printf("can't open %s",UP_RECORD_PATH);  
        return false;
    }
    else
    {
        while (((dirp=readdir(dp))!=NULL) && (n<=50))    
        {       
            n++;    
            if (strcmp(dirp->d_name, ".") == 0 ||strcmp(dirp->d_name, "..") == 0 ) 
            {}
            else
            {         
                int nLen = strlen(dirp->d_name);
                int nPathLen = strlen(UP_RECORD_PATH);
                if(nLen > 5)
                {
                    memcpy(pFilePathName,UP_RECORD_PATH,nPathLen);
                    memcpy(&pFilePathName[nPathLen],dirp->d_name,nLen);
                    memcpy(pFileName,dirp->d_name,nLen);

                    if (strcmp(g_CurLogFileName,pFilePathName) == 0)
                    {}
                    else
                    {
                        closedir(dp); 
                        printf("EnumRecordFile File = %s \r\n",pFileName);
                        return true;
                    }  
                } 
            }
        }    
        closedir(dp);  
    }

    return false;
}

boolean OpenUpRecordFile(char* FileName,unsigned int* dwFileSize,unsigned int* dwBlockSize,unsigned int* dwFileChecksum)
{
    if(g_CurOpenUpRecordFile)
    {
        printf("befor g_CurOpenUpRecordFile is not NULL  \r\n");
        return false;
    }
 
	g_CurOpenUpRecordFile = fopen(FileName,"rb");
	if(g_CurOpenUpRecordFile == NULL)
	{
        printf("after g_CurOpenUpRecordFile is NULL  %s \r\n",FileName);
		return false;
	}
	
	fseek(g_CurOpenUpRecordFile,0,SEEK_END);
	*dwFileSize = ftell(g_CurOpenUpRecordFile);
	rewind(g_CurOpenUpRecordFile);

    *dwBlockSize = (*dwFileSize) / 25;
    *dwFileSize  = (*dwBlockSize) * 25;

    memset(g_CurTxUpRecordFileName,0,256);
    memcpy(g_CurTxUpRecordFileName,FileName,strlen(FileName));
    g_dwCurTxUpRecordFileSize = *dwFileSize;
    g_dwCurTxUpRecordBlockSize = *dwBlockSize;
    g_dwCurTxUpRecordBlockIndex = 0;
    unsigned int i = 0;
    char ch = 0;
    g_dwCurTxUpRecordFileChecksum = 0;
    for(i=0;i<g_dwCurTxUpRecordFileSize;i++)
    {
        ch = fgetc(g_CurOpenUpRecordFile);
        g_dwCurTxUpRecordFileChecksum = g_dwCurTxUpRecordFileChecksum + ch;
    }
    g_dwCurTxUpRecordFileChecksum = ~g_dwCurTxUpRecordFileChecksum;
    *dwFileChecksum = g_dwCurTxUpRecordFileChecksum;
    rewind(g_CurOpenUpRecordFile);

    printf("OpenUpRecordFile file = %s  \r\n",FileName);

    return true;
}

void CloseCurOpenedUpRecordFile()
{
    if(g_CurOpenUpRecordFile)
    {
        fclose(g_CurOpenUpRecordFile);
        g_CurOpenUpRecordFile = NULL;
        memset(g_CurTxUpRecordFileName,0,256);
        g_dwCurTxUpRecordFileSize = 0;
        g_dwCurTxUpRecordFileChecksum = 0;
        g_dwCurTxUpRecordBlockSize = 0;
        g_dwCurTxUpRecordBlockIndex = 0;
    }
}

boolean GetFileTxBlockData(unsigned int dwBlockIndex,unsigned char* pData)
{
    if(g_CurOpenUpRecordFile)
    {
        if(dwBlockIndex != g_dwCurTxUpRecordBlockIndex)
        {
            printf("GetFileTxBlockData dwBlockIndex = %d g_dwCurTxUpRecordBlockIndex = %d \r\n",dwBlockIndex,g_dwCurTxUpRecordBlockIndex);
            return false;
        }
        if(g_dwCurTxUpRecordBlockIndex == g_dwCurTxUpRecordBlockSize)
        {
            printf("GetFileTxBlockData dwBlockIndex = %d g_dwCurTxUpRecordBlockSize = %d \r\n",dwBlockIndex,g_dwCurTxUpRecordBlockSize);
            return false;
        }
        g_dwCurTxUpRecordBlockIndex++;
		fread(pData,25,1,g_CurOpenUpRecordFile);
        return true;
    }
    printf("GetFileTxBlockData g_CurOpenUpRecordFile = NULL\r\n");
    return false;
}

void SaveUpRecordFileMirror()
{
    FILE* MirrorFile = fopen(UP_RECORD_MIRROR_FILE,"w+b");	
	if(MirrorFile != NULL)
	{
        fwrite(g_CurTxUpRecordFileName,256,1,MirrorFile);
        unsigned char pBuff[25] = {0};
        pBuff[0] = (unsigned char)(g_dwCurTxUpRecordFileSize&0x000000FF);
        pBuff[1] = (unsigned char)((g_dwCurTxUpRecordFileSize&0x0000FF00)>>8);
        pBuff[2] = (unsigned char)((g_dwCurTxUpRecordFileSize&0x00FF0000)>>16);
        pBuff[3] = (unsigned char)((g_dwCurTxUpRecordFileSize&0xFF000000)>>24);

        pBuff[4] = (unsigned char)(g_dwCurTxUpRecordFileChecksum&0x000000FF);
        pBuff[5] = (unsigned char)((g_dwCurTxUpRecordFileChecksum&0x0000FF00)>>8);
        pBuff[6] = (unsigned char)((g_dwCurTxUpRecordFileChecksum&0x00FF0000)>>16);
        pBuff[7] = (unsigned char)((g_dwCurTxUpRecordFileChecksum&0xFF000000)>>24);

        pBuff[8] = (unsigned char)(g_dwCurTxUpRecordBlockSize&0x000000FF);
        pBuff[9] = (unsigned char)((g_dwCurTxUpRecordBlockSize&0x0000FF00)>>8);
        pBuff[10] = (unsigned char)((g_dwCurTxUpRecordBlockSize&0x00FF0000)>>16);
        pBuff[11] = (unsigned char)((g_dwCurTxUpRecordBlockSize&0xFF000000)>>24);

        pBuff[12] = (unsigned char)(g_dwCurTxUpRecordBlockIndex&0x000000FF);
        pBuff[13] = (unsigned char)((g_dwCurTxUpRecordBlockIndex&0x0000FF00)>>8);
        pBuff[14] = (unsigned char)((g_dwCurTxUpRecordBlockIndex&0x00FF0000)>>16);
        pBuff[15] = (unsigned char)((g_dwCurTxUpRecordBlockIndex&0xFF000000)>>24);
        
        fwrite(pBuff,16,1,MirrorFile);
        fclose(MirrorFile);
	}
}

boolean LoadUpRecordFileMirror(unsigned int* dwCurBlockIndex)
{
    if(g_CurOpenUpRecordFile)
    {
        return false;
    }

    FILE* MirrorFile = fopen(UP_RECORD_MIRROR_FILE,"rb");	
	if(MirrorFile != NULL)
    {
        fread(g_CurTxUpRecordFileName,256,1,MirrorFile);
        unsigned char pBuff[25] = {0};
        fread(pBuff,16,1,MirrorFile);
        g_dwCurTxUpRecordFileSize = pBuff[3]; 
        g_dwCurTxUpRecordFileSize = g_dwCurTxUpRecordFileSize << 8;
        g_dwCurTxUpRecordFileSize = g_dwCurTxUpRecordFileSize|pBuff[2]; 
        g_dwCurTxUpRecordFileSize = g_dwCurTxUpRecordFileSize << 8;
        g_dwCurTxUpRecordFileSize = g_dwCurTxUpRecordFileSize|pBuff[1]; 
        g_dwCurTxUpRecordFileSize = g_dwCurTxUpRecordFileSize << 8;
        g_dwCurTxUpRecordFileSize = g_dwCurTxUpRecordFileSize|pBuff[0]; 

        g_dwCurTxUpRecordFileChecksum = pBuff[7]; 
        g_dwCurTxUpRecordFileChecksum = g_dwCurTxUpRecordFileChecksum << 8;
        g_dwCurTxUpRecordFileChecksum = g_dwCurTxUpRecordFileChecksum|pBuff[6]; 
        g_dwCurTxUpRecordFileChecksum = g_dwCurTxUpRecordFileChecksum << 8;
        g_dwCurTxUpRecordFileChecksum = g_dwCurTxUpRecordFileChecksum|pBuff[5]; 
        g_dwCurTxUpRecordFileChecksum = g_dwCurTxUpRecordFileChecksum << 8;
        g_dwCurTxUpRecordFileChecksum = g_dwCurTxUpRecordFileChecksum|pBuff[4]; 

        g_dwCurTxUpRecordBlockSize = pBuff[11]; 
        g_dwCurTxUpRecordBlockSize = g_dwCurTxUpRecordBlockSize << 8;
        g_dwCurTxUpRecordBlockSize = g_dwCurTxUpRecordBlockSize|pBuff[10]; 
        g_dwCurTxUpRecordBlockSize = g_dwCurTxUpRecordBlockSize << 8;
        g_dwCurTxUpRecordBlockSize = g_dwCurTxUpRecordBlockSize|pBuff[9]; 
        g_dwCurTxUpRecordBlockSize = g_dwCurTxUpRecordBlockSize << 8;
        g_dwCurTxUpRecordBlockSize = g_dwCurTxUpRecordBlockSize|pBuff[8]; 

        g_dwCurTxUpRecordBlockIndex = pBuff[15]; 
        g_dwCurTxUpRecordBlockIndex = g_dwCurTxUpRecordBlockIndex << 8;
        g_dwCurTxUpRecordBlockIndex = g_dwCurTxUpRecordBlockIndex|pBuff[14]; 
        g_dwCurTxUpRecordBlockIndex = g_dwCurTxUpRecordBlockIndex << 8;
        g_dwCurTxUpRecordBlockIndex = g_dwCurTxUpRecordBlockIndex|pBuff[13]; 
        g_dwCurTxUpRecordBlockIndex = g_dwCurTxUpRecordBlockIndex << 8;
        g_dwCurTxUpRecordBlockIndex = g_dwCurTxUpRecordBlockIndex|pBuff[12]; 

        fclose(MirrorFile);

        if (0 == g_dwCurTxUpRecordFileSize) 
        {
            return false;
        }

        if((g_dwCurTxUpRecordBlockSize*25) > g_dwCurTxUpRecordFileSize)
        {
            return false;
        }
        
        if(g_dwCurTxUpRecordBlockIndex >= g_dwCurTxUpRecordBlockSize)
        {
            return false;
        }

        g_CurOpenUpRecordFile = fopen(g_CurTxUpRecordFileName,"rb");
        if(g_CurOpenUpRecordFile == NULL)
        {
            return false;
        }

        *dwCurBlockIndex = g_dwCurTxUpRecordBlockIndex;
        return true;
    }

    return false;
}

void DeleteRecordFile(char* pFileName)
{
    char cmd[256] = "/bin/rm -rf /media/sd-mmcblk0p1/record/";

    int ncmdLen = strlen(cmd);
    int nlen = strlen(pFileName);
    memcpy(&cmd[ncmdLen],pFileName,nlen);
    system(cmd);
    printf("DeleteRecordFile rm -> %s \n",cmd);
    sleep(1);


/*
    int nLen = strlen(pFileName);
    int nPathLen = strlen(UP_RECORD_PATH);
    char pFilePathName[256] = {0};

    memcpy(pFilePathName,UP_RECORD_PATH,nPathLen);
    memcpy(&pFilePathName[nPathLen],pFileName,nLen);
    remove(pFilePathName);*/
}

void CopyFileToPath(char* SrcFilePathName,char* DstPath)
{
    char cmd[256] = "cp ";

    int ncmdLen = strlen(cmd);
    int nSrcFilePathNameLen = strlen(SrcFilePathName);
    int nDstPathLen = strlen(DstPath);

    memcpy(&cmd[ncmdLen],SrcFilePathName,nSrcFilePathNameLen);
    cmd[ncmdLen + nSrcFilePathNameLen] = ' ';
    memcpy(&cmd[ncmdLen + nSrcFilePathNameLen + 1],DstPath,nDstPathLen);
    system(cmd);
    printf("%s \r\n",cmd);
    sleep(5);
}
