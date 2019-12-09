
#include "DaqService.h"
#include "DaqConfig.h"
#include "systemtimer.h"
#include <stdio.h>
#include "RawCanMsgLogFile.h"
#include "wifi_esp8266.h"

volatile boolean g_bRecvCalOdtMsgThreadRun = false;
pthread_t g_nRecvCalOdtMsgThreadId = 0;



 

void ThreadRecvCalOdtMsg()
{
    int i = 0;
    while(g_bRecvCalOdtMsgThreadRun)
    {
         
       // usleep(500000);
        pthread_mutex_lock(&g_CalOdt[0].mutex);
        for(i=0;i<256;i++)
        {
            if(g_CalOdt[0].odt[i].bActive)
            {
        
                g_CalOdt[0].odt[i].msg.dbTimestamp = GetSystemTimeMs();
                SetLogRawMsg(&g_CalOdt[0].odt[i].msg);
            }
        }
        pthread_mutex_unlock(&g_CalOdt[0].mutex);

        pthread_mutex_lock(&g_CalOdt[1].mutex);
        for(i=0;i<256;i++)
        {
            if(g_CalOdt[1].odt[i].bActive)
            {
                g_CalOdt[1].odt[i].msg.dbTimestamp = GetSystemTimeMs();
                SetLogRawMsg(&g_CalOdt[1].odt[i].msg);
            }
        }
        pthread_mutex_unlock(&g_CalOdt[1].mutex);
    }
}

void InitSetDaqServiceOnCanConfig(int nCanChanIndex)
{
 
    if(g_CalDaqCfgFile.m_CfgCanChanDaqSer[nCanChanIndex].ucEnable)
    {

        SetCanChanEnable(nCanChanIndex, true);
        SetCanChanBaudrate(0,g_CalDaqCfgFile.m_CfgCanChanDaqSer[nCanChanIndex].wBaudrate);

        
        unsigned int dwMsgID = 0;
        dwMsgID = g_CalDaqCfgFile.m_CfgCanChanDaqSer[nCanChanIndex].dwCmdTxMsgID;
        if(0xFFFFFFFF == dwMsgID)
        {
            SetCanChanRxFilter(nCanChanIndex,0,0xFFFFFFFF,true);
        }
        else 
        {
            if(dwMsgID&0x80000000)
            {
                SetCanChanRxFilter(nCanChanIndex,0,dwMsgID&0x1FFFFFFF,true);
            }
            else
            {
                SetCanChanRxFilter(nCanChanIndex,0,dwMsgID&0x000007FF,false);
            }
        }

        dwMsgID = g_CalDaqCfgFile.m_CfgCanChanDaqSer[nCanChanIndex].dwCmdRxMsgID;
        if(0xFFFFFFFF == dwMsgID)
        {
            SetCanChanRxFilter(nCanChanIndex,1,0xFFFFFFFF,true);
        }
        else 
        {
            if(dwMsgID&0x80000000)
            {
                SetCanChanRxFilter(nCanChanIndex,1,dwMsgID&0x1FFFFFFF,true);
            }
            else
            {
                SetCanChanRxFilter(nCanChanIndex,1,dwMsgID&0x000007FF,false);
            }
        }

        int index = 0;
        int i = 0;
        for(i=0;i<5;i++)
        {
            if (g_CalDaqCfgFile.m_CfgCanChanDaqSer[nCanChanIndex].DaqListChan[i].bIsEnable) 
            {
                dwMsgID = g_CalDaqCfgFile.m_CfgCanChanDaqSer[nCanChanIndex].DaqListChan[i].dwDaqRxMsgID;
                if(dwMsgID&0x80000000)
                {
                    SetCanChanRxFilter(nCanChanIndex,2+index,dwMsgID&0x1FFFFFFF,true);
                }
                else
                {
                    SetCanChanRxFilter(nCanChanIndex,2+index,dwMsgID&0x000007FF,false);
                }
                index++;
            }
        }
        SetCanChanRxFilter(nCanChanIndex,2+index,0x18DF0001,true);   
          
    }
}



boolean InitDaqService()
{
    if(!IsDaqConfigInitOK())
    {
        printf("Daq Service Init failed : Daq config is not Init  \r\n");
        return false;
    }

    InitSetDaqServiceOnCanConfig(0);
    InitSetDaqServiceOnCanConfig(1);

    if(!InitCan())
    {
        printf("System Init : InitCan failed \n");
        return false;
    }

     
 
    
    if(g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].ucEnable)
    {
        g_RecvCanBusMsgFlag[0] = true;
        #if 0
        if(0 == g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].ucCalDaqType)
        {
           // if(!InitCcpService(0))
          //  {
            //    printf("Daq Service Init : channel 0 ccp init failed \n");
              //  return false;
          //  }
          //  printf("Daq Service Init : channel 0 ccp init successful \n");
        }
        else if(1 == g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].ucCalDaqType)
        {
           // if(!InitXcpService(0))
           // {
             //   printf("Daq Service Init : channel 0 Xcp init failed \n");
               // return false;
          //  }
           // printf("Daq Service Init : channel 0 Xcp init successful \n");
        }
        else
        {//SAE-J1939
            g_RecvCanBusMsgFlag[0] = true;
            /* code */
        }
        #endif
        g_dbLastRecvCalOdtMsgTimestamp[0] = GetSystemTimeMs();
    }



    if(g_CalDaqCfgFile.m_CfgCanChanDaqSer[1].ucEnable)
    {
        if(0 == g_CalDaqCfgFile.m_CfgCanChanDaqSer[1].ucCalDaqType)
        {
           // if(!InitCcpService(1))
           // {
           //     printf("Daq Service Init : channel 1 ccp init failed \n");
           //     return false;
           // }
           // printf("Daq Service Init : channel 1 ccp init successful \n");
        }
        else if(1 == g_CalDaqCfgFile.m_CfgCanChanDaqSer[1].ucCalDaqType)
        {
          //  if(!InitXcpService(1))
           // {
           //     printf("Daq Service Init : channel 1 Xcp init failed \n");
           //     return false;
          //  }
           // printf("Daq Service Init : channel 1 Xcp init successful \n");
        }
        else
        {//SAE-J1939
            g_RecvCanBusMsgFlag[1] = true;
            /* code */
        }
        
        g_dbLastRecvCalOdtMsgTimestamp[1] = GetSystemTimeMs();
    }

    if(!g_bRecvCalOdtMsgThreadRun)
    {
        g_bRecvCalOdtMsgThreadRun = true;
        int result = pthread_create(&g_nRecvCalOdtMsgThreadId,NULL,(void *)ThreadRecvCalOdtMsg,NULL);
        if(0 != result)
        {
            g_bRecvCalOdtMsgThreadRun = false;
            return false;
        }
    }

      


    return true;
}

void ExitDaqService()
{
    if(g_bRecvCalOdtMsgThreadRun)
    {
        g_bRecvCalOdtMsgThreadRun = false;
        pthread_join(g_nRecvCalOdtMsgThreadId, NULL);
        g_nRecvCalOdtMsgThreadId = NULL;
    }
   // ExitCcpService();
   // ExitXcpService();
    g_RecvCanBusMsgFlag[0] = false;
	g_RecvCanBusMsgFlag[1] = false;
    ExitCan();
}

boolean IsDaqRecvMsgTimeout()
{
    double dbCurTime = GetSystemTimeMs();
    if(g_CalDaqCfgFile.m_CfgCanChanDaqSer[0].ucEnable&&g_RecvCalOdtMsgFlag[0])
    {
        if((dbCurTime - g_dbLastRecvCalOdtMsgTimestamp[0]) > DAQ_RECV_MSG_TIME_OUT_MAX)
        {
            return true;
        }
    }

    if(g_CalDaqCfgFile.m_CfgCanChanDaqSer[1].ucEnable&&g_RecvCalOdtMsgFlag[1])
    {
        double dbCurTime = GetSystemTimeMs();
        if((dbCurTime - g_dbLastRecvCalOdtMsgTimestamp[1]) > DAQ_RECV_MSG_TIME_OUT_MAX)
        {
            return true;
        }
    }

    return false;
}