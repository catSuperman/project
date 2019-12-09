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
#include <string.h>

#include "SignalConfig.h"


#define TYPEFLAG "SignalTypeMap"
#define ENUMFLAG "SignalEnumMap"
#define BLANK ' '
#define DAQ_CONFIG_BASE_FILE  "SignalConfig.txt"
char g_SignalConfigFileName[64] = {0};

    int stop_flag = 1;
    int ItemFlag = 0;
 

void Detect_Map_Seed_Data(char * rdbuf)
{
    char tmp[20];
    int flag_i = 0;
    int i = 0;
        while(rdbuf[i] != '\n')
        {
            flag_i++;
            i++;
        }
        memset(tmp,0,20);
        strncpy(tmp,&rdbuf[1],1);
        SIG_CFG.MapItem[ItemFlag].Signal_Key = atoi(tmp);
        memset(tmp,0,20);
        strncpy(tmp,&rdbuf[2],flag_i-3);
        strcpy(SIG_CFG.MapItem[ItemFlag].Signal_Value,tmp);
        ItemFlag++;
}


void Detect_Map_Seed(FILE * fptr,char * rdbuf)
{
    int i=0;
    fgets(rdbuf,100,fptr);
    while(stop_flag)
    {
        if((strncmp(rdbuf,ENUMFLAG,13)))
        {
            printf("rdbuf : %s\n",&rdbuf[0]);
            Detect_Map_Seed_Data(rdbuf);
            fgets(rdbuf,100,fptr);
        }  
        else
        {
            stop_flag=0;
        }
    }
    
}

void *Detect_Map(FILE * fptr,char * rdbuf)
{
    fgets(rdbuf,100,fptr);
    if(0 == (strncmp(rdbuf,TYPEFLAG,13)))
    {
        strcpy(SIG_CFG.SignalTypeMap,rdbuf);
        printf("TYPE1  : %s\n",SIG_CFG.SignalTypeMap); 
        return(rdbuf);
    }
    return NULL;
}



boolean SignalBaseConfig()
{
    memset(g_SignalConfigFileName,0,64);
    char temp[1024] = {0};
    char SIG_cfg_buf[100] = {0};
    FILE *fp = NULL;
    int iCount = 0;
    int i = 0;
    char ch = 0;
    fp = fopen(DAQ_CONFIG_BASE_FILE,"rb");
    if(fp == NULL)
    {
        return false;
    }
    while(stop_flag)
    {
        printf("********qwe\n");
        if(Detect_Map(fp,SIG_cfg_buf) != NULL)
        {     
            Detect_Map_Seed(fp,SIG_cfg_buf);  
        }
    }
    return 1;
}