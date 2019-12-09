#include "DBC_SignalExplain.h"
#include <time.h>
#include <unistd.h>
#include "4g_sim7600.h"
#include "wifi_esp8266.h"
#include "SignalConfig.h"

volatile boolean g_bRxQueueThreadRun = 0;


pthread_t g_nCanThreadId = 0;

pthread_mutex_t g_WritDbcFileMutex;
SIGNAL_t g_SIGNAL;

SIGNAL_TYPE_QUEUE g_TxSignalDataQueue;

#define CAN_DATA_SIZE 5
 unsigned char sendData[1024] = {0}; 
 int SignalStrlen = 0;
extern int SendDataLen;
extern int SendDataLen4g;
 double startTime = 0;
double presentTime = 0;
int SignalTypeMAX = -1;
int signalNumFlag = 0;


void sendDataToSer();

/*���źŷ�Ϊ3���࣬һ��Ϊ���ͱ�ʶ
0:Vehicle      ���� 
1:BMS          ���
2:Motor        ���
*/
int Vehicle=0;
int BMS=1;
int Motor=2;

int VehicleFlag=0;
int BMSFlag=0;
int MotorFlag=0;






/*
��ȡ����λ�Ķ�����1��mask
int nbits������λ
����ֵ��maskֵ
*/
int bit_mask(int nbits)
{
    int i;
    int ret = 1;
    for(i = 0; i < nbits -1;i++)
    {
        ret = ret * 2+1;
    }
    return ret;
}

    //if the signal cross bytes
int CrossByte(int start, int size)
{
    int tmp = (start % 8)+1 -size;

    if( tmp < 0 )
    {
        if(tmp < -16)
        {
            return 4;
        }
        else if(tmp < -8)
        {
            return 3;
        }
        else
            return 2;
    }
    return 0;
}

int CrossBytesLocation(int start, int size,int crossbyte)
{
    int offset = 0;
    if(crossbyte == 2)
        offset = 8;
    else if(crossbyte == 3)
        offset = 16;
    else if(crossbyte == 4)
        offset = 24;
    int ret = 1 + offset -(size - (start % 8));
    return ret ;
}

void InitDbcRawMsgQueue(SIGNAL_TYPE_QUEUE * Queue)
{
    pthread_mutex_init(&Queue->m_mutex,NULL);
    Queue->m_nCnt = 0;
    Queue->m_nRd = 0;
    Queue->m_nWt = 0;
}



boolean OutDbcRawMsgQueue(SIGNAL_TYPE_QUEUE * Queue, SIGNAL_t * SignalMsg)
{
   
    pthread_mutex_lock(&Queue->m_mutex);
    if(Queue->m_nCnt <= 0)
    {
        pthread_mutex_lock(&Queue->m_mutex);
        return 0;
    }
    printf("out queue\n");
    memcpy(SignalMsg,&Queue->SIGNAL_L[Queue->m_nRd],sizeof(Queue->SIGNAL_L[Queue->m_nRd]));
    Queue->m_nRd++;
    if(Queue->m_nRd >= RAW_MSG_QUEUE_SIZE_dbc)
    {
        Queue->m_nRd = 0;
    }
    Queue->m_nCnt--;
    pthread_mutex_unlock(&Queue->m_mutex);
   
    return 1;
}

int i,j = 0;


struct timespec g_timer_curr;
struct timespec g_timer_start;

int GetSystemTimeMss()
{
    clock_gettime(CLOCK_MONOTONIC, &g_timer_curr); 
    int dbTimestamp = (g_timer_curr.tv_sec - g_timer_start.tv_sec) * 1000 + (g_timer_curr.tv_nsec - g_timer_start.tv_nsec)/1000000;
    return dbTimestamp;
}

int g_bTxThreadRun = 0;


void ThreadCanSendMsg()
{
    while(g_bTxThreadRun)
    {
        sendDataToSer();
    }
}


void InitInterpreter()
{
    int i;

    printf("*******queue init\n");
    pthread_mutex_init(&g_WritDbcFileMutex, NULL);
    int result = 0;
    InitDbcRawMsgQueue(&g_TxSignalDataQueue);	

    g_bTxThreadRun = 1;
    result = pthread_create(&g_nCanThreadId,NULL,(void *)ThreadCanSendMsg,NULL);
    if(0 != result)
	{
		g_bTxThreadRun = 0;
		return 0;
	}		
    //��ȡ��ʼʱ��
    startTime = GetSystemTimeMs();
    for(i = 0; i< 20;i++)
    {
        SIG_CFG.MapItem[i].VehicleFlag = 0;
    }
     for(i = 0; i< 10;i++)
    {
        DataToSerMsg[i].sendToSerStrlen = 0;
    }
}



void InDbcRawMsgQueue(SIGNAL_TYPE_QUEUE * Queue, SIGNAL_t * SignalMsg)
{
    printf("in queue\n");
    g_bRxQueueThreadRun = 1;
    pthread_mutex_lock(&Queue->m_mutex);
    memcpy(&Queue->SIGNAL_L[Queue->m_nWt],SignalMsg,sizeof(Queue->SIGNAL_L[Queue->m_nWt]));
    Queue->m_nWt++;
    if(Queue->m_nWt >= RAW_MSG_QUEUE_SIZE_dbc)
    {
        Queue->m_nWt = 0;
    }
    Queue->m_nCnt++;
    if(Queue->m_nCnt > RAW_MSG_QUEUE_SIZE_dbc)
    {
        Queue->m_nCnt = RAW_MSG_QUEUE_SIZE_dbc;
        Queue->m_nRd++;
        if(Queue->m_nRd >= RAW_MSG_QUEUE_SIZE_dbc)
        {
            Queue->m_nRd = 0;
        }
    }
    pthread_mutex_unlock(&Queue->m_mutex);

}

boolean IsDbcRawMsgQueueFull(SIGNAL_TYPE_QUEUE* Queue)
{
	boolean bFull = 0;
	pthread_mutex_lock(&Queue->m_mutex);
	if(Queue->m_nCnt >= RAW_MSG_QUEUE_SIZE_dbc)
	{
		bFull = 1;
	}
	pthread_mutex_unlock(&Queue->m_mutex);
	return bFull;
}




void SendCanMsgToServer(SEND_FRAME_MSG RX_Frame,int signalNum,int type)
{
    int k;
   
    unsigned char parity = 0;
    RX_Frame.frame_head[0] = 0xFF;   //֡ͷ
    RX_Frame.frame_head[1] = 0xFD;
    RX_Frame.frame_type[0] = 0;
    RX_Frame.frame_type[0] |= (unsigned char)((type << 4) );


    RX_Frame.frame_type[0] |= (unsigned char)((SignalStrlen >> 8) & 0x0F);
    RX_Frame.frame_type[1]  = (unsigned char)(SignalStrlen & 0x00FF);

    //Ӳ����ʶ
    RX_Frame.deviceId[0] = 0x79;
    RX_Frame.deviceId[1] = 0x78;
    RX_Frame.deviceId[2] = 0x30;
    RX_Frame.deviceId[3] = 0x31;
   
   
    time_t t;
	t = time(NULL);
 
	int ii = time(&t);
    int* p = &ii;
    unsigned char* c = (unsigned char*)p;


    
    memset(sendData,0,1024);

	//printf("ii = %d\n", ii);


    memcpy(&sendData[0],&RX_Frame.frame_head[0],2);
    memcpy(&sendData[2],&RX_Frame.frame_type[0],2);

    memcpy(&sendData[4],&RX_Frame.deviceId[0],4);

    sendData[8] = c[0];
	sendData[9]= c[1];
	sendData[10] = c[2];
	sendData[11] = c[3];
	sendData[12] = c[4];
	sendData[13] = c[5];
	sendData[14] = c[6];
	sendData[15] = c[7];

    for(k = 0; k < 8; k++)
    {
         parity ^= sendData[8+k];

    }
  
    //������
    for(i = 0; i < signalNum;i++)
    {
       memcpy(&sendData[16+3*i],&RX_Frame.data[i].signal_type,1);
       memcpy(&sendData[17+3*i],&RX_Frame.data[i].signal_identifying,1);
       memcpy(&sendData[18+3*i],&RX_Frame.data[i].signal,1);
       parity ^= (unsigned char)RX_Frame.data[i].signal_type;
       parity ^= (unsigned char)RX_Frame.data[i].signal_identifying;
       parity ^= (unsigned char)RX_Frame.data[i].signal;
    }
   

    RX_Frame.parityBit = RX_Frame.frame_type[0] ^ RX_Frame.frame_type[1] ^ RX_Frame.deviceId[0] ^ RX_Frame.deviceId[1] ^ RX_Frame.deviceId[2] ^ RX_Frame.deviceId[3] ^ parity;
    memcpy(&sendData[16+signalNum * 3],&RX_Frame.parityBit,1);

   printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  %02x\n",sendData[0],sendData[1],sendData[2],sendData[3],sendData[4],sendData[5],
   sendData[6],sendData[7],sendData[8],sendData[9],sendData[10],sendData[11],sendData[12],sendData[13],sendData[14],sendData[15],sendData[16],sendData[17],sendData[18],sendData[19],sendData[20],sendData[21],sendData[22],sendData[23]
   ,sendData[24],sendData[25],sendData[26],sendData[27],sendData[28],sendData[29],sendData[30],sendData[31]);
   sendCmdWifi2(sendData);
   //sendCmd4g_byte(sendData);
 
}




 
void DataPackaging()
{
    //pthread_mutex_lock(&g_WritDbcFileMutex);
    printf("start pthread\n");
      // SIGNAL_t g_SIGNAL;
      SIGNAL_t g_SIGNALDate;
      SEND_FRAME_MSG RX_Frame;
       while(1)
       {
           printf("out queue\n");
           while(OutDbcRawMsgQueue(&g_TxSignalDataQueue,&g_SIGNALDate))
           {
            printf("**************************************************************************\n");
            RX_Frame.frame_head[0] = 0xFF;   //֡ͷ
            RX_Frame.frame_head[1] = 0xFD;
            printf("signal : %d  signal_type : %d signal_identifying : %d signal_strlen  : %d\n",g_SIGNALDate.signal,g_SIGNALDate.signal_type,g_SIGNALDate.signal_identifying,g_SIGNALDate.signal_strlen);
            
            RX_Frame.frame_type[0] = 0;
            RX_Frame.frame_type[0] |= (unsigned char)((g_SIGNALDate.signal_type << 4) );
            RX_Frame.frame_type[0] |= (unsigned char)((g_SIGNALDate.signal_strlen >> 8) & 0x0F);
            RX_Frame.frame_type[1]  = (unsigned char)(g_SIGNALDate.signal_strlen & 0x00FF);

            //Ӳ����ʶ
            RX_Frame.deviceId[0] = 0xA9;
            RX_Frame.deviceId[1] = 0x8A;
            RX_Frame.deviceId[2] = 0xC7;
            RX_Frame.deviceId[3] = 0;

            //������
            RX_Frame.data[j].signal_type = (g_SIGNALDate.signal_type << 4);
            RX_Frame.data[j].signal_type |= (1 << 3);
            RX_Frame.data[j].signal_type |=  g_SIGNALDate.signal_strlen;

            RX_Frame.data[j].signal_identifying = i;

            RX_Frame.data[j].signal = g_SIGNALDate.signal;

    
            printf("RX_Frame.frame_type %02X  %02X  %02X \n",RX_Frame.frame_type[0],RX_Frame.frame_type[1],RX_Frame.data[0].signal_type);

            //SendCanMsgToServer(RX_Frame);
        
            }  
          // g_bRxQueueThreadRun = 0;

       }
     // pthread_mutex_unlock(&g_WritDbcFileMutex);
    
}



void DBC_CanDataPack(SEND_FRAME_MSG *RX_Frame,SIGNAL_t * g_SIGNALDate,int signalFlagNum)
{
    printf("******* signalFlagNum : %d\n",signalFlagNum);
    printf("signal : %d  signal_type : %d signal_identifying : %d signal_strlen  : %d\n",g_SIGNALDate->signal,g_SIGNALDate->signal_type,g_SIGNALDate->signal_identifying,g_SIGNALDate->signal_strlen);
    

    printf("**************************************************************************\n");
          //  RX_Frame->frame_head[0] = 0xFF;   //֡ͷ
           // RX_Frame->frame_head[1] = 0xFD;
           // printf("signal : %d  signal_type : %d signal_identifying : %d signal_strlen  : %d\n",g_SIGNALDate->signal,g_SIGNALDate->signal_type,g_SIGNALDate->signal_identifying,g_SIGNALDate->signal_strlen);
            
           // RX_Frame->frame_type[0] = 0;
           // RX_Frame->frame_type[0] |= (unsigned char)((g_SIGNALDate->signal_type << 4) );
            //RX_Frame->frame_type[0] |= (unsigned char)((g_SIGNALDate->signal_strlen >> 8) & 0x0F);
            //RX_Frame->frame_type[1]  = (unsigned char)(g_SIGNALDate->signal_strlen & 0x00FF);


  

            //Ӳ����ʶ
           // RX_Frame->deviceId[0] = 0xA9;
           // RX_Frame->deviceId[1] = 0x8A;
           // RX_Frame->deviceId[2] = 0xC7;
           // RX_Frame->deviceId[3] = 0;

            //������
            RX_Frame->data[signalFlagNum].signal_type = (unsigned char)(g_SIGNALDate->signal_type << 4);
            RX_Frame->data[signalFlagNum].signal_type |= (g_SIGNALDate->Realtime_Show_flag << 3);
            RX_Frame->data[signalFlagNum].signal_type |=  g_SIGNALDate->signal_strlen;

            RX_Frame->data[signalFlagNum].signal_identifying = g_SIGNALDate->signal_identifying;
           
            printf("g_SIGNALDate->signal_type : %d\n",g_SIGNALDate->signal_type);
            printf("g_SIGNALDate->signal : %d\n",g_SIGNALDate->signal);
            RX_Frame->data[signalFlagNum].signal = g_SIGNALDate->signal;

       
            printf("RX_Frame.frame_type %02X  %02X  %02X \n",RX_Frame->frame_type[0],RX_Frame->frame_type[1],RX_Frame->data[signalFlagNum].signal_type);
      
}

int SendDataStrlen = 0;

void SendCanMsgToServerMsg(SEND_FRAME_TO_SER_MSG  RX_Frame,int signalNum,int type)
{
    int k,s=0;
    int j=0;
   
    unsigned char parity = 0;
    SendDataLen4g = 0;
    //Ӳ����ʶ
    RX_Frame.deviceId[0] = 0x79;
    RX_Frame.deviceId[1] = 0x78;
    RX_Frame.deviceId[2] = 0x30;
    RX_Frame.deviceId[3] = 0x31;
   
    time_t t;
	t = time(NULL);
	int ii = time(&t);
    int* p = &ii;
    unsigned char* c = (unsigned char*)p;
    memset(sendData,0,1024);

    //ʱ���
    sendData[8] = c[0];
	sendData[9]= c[1];
	sendData[10] = c[2];
	sendData[11] = c[3];
	sendData[12] = c[4];
	sendData[13] = c[5];
	sendData[14] = c[6];
	sendData[15] = c[7];

    printf("signalNum : %d\n",signalNum);
     //������
    printf("SendDataStrlen **** : %d\n",SendDataStrlen);
    for(i = 0; i <= CAN_DATA_SIZE ;i++)
    {
       memcpy(&sendData[16+3*i+j],&RX_Frame.data[i].signal_type,1);
       memcpy(&sendData[17+3*i+j],&RX_Frame.data[i].signal_identifying,1);
       parity ^= (unsigned char)RX_Frame.data[i].signal_type;
       parity ^= (unsigned char)RX_Frame.data[i].signal_identifying;

       SendDataLen4g += RX_Frame.data[i].sig_strlen;

       if(RX_Frame.data[i].signal_num_flag)
       {
           memcpy(&sendData[18+3*i+j],&RX_Frame.data[i].SignalDataDouble,2);
           parity ^= (unsigned char)sendData[18+3*i+j];
           parity ^= (unsigned char)sendData[18+3*i+j+1];
           //s+=4;
       }
       else
       {
           memcpy(&sendData[18+3*i+j],&RX_Frame.data[i].SignalDateOne,1);
           parity ^= (unsigned char)sendData[18+3*i+j];
           //s+=3;
           j--; 
       }
       j++;
        
    }
    printf("s    strlen : %d\n",s);
    int Len =  16 + 1;
    SendDataLen4g += Len;
    printf("SendDataLen4g : %d\n",SendDataLen4g);
    int frameLen = SendDataLen4g - 5;

    RX_Frame.frame_head[0] = 0xFF;   //֡ͷ
    RX_Frame.frame_head[1] = 0xFD;     
    RX_Frame.frame_type[0] = 0;
    RX_Frame.frame_type[0] |= (unsigned char)((type << 4) );
    RX_Frame.frame_type[0] |= (unsigned char)(((frameLen ) >> 8) & 0x0F);
    RX_Frame.frame_type[1]  = (unsigned char)((frameLen ) & 0x00FF);

    memcpy(&sendData[0],&RX_Frame.frame_head[0],2);
    memcpy(&sendData[2],&RX_Frame.frame_type[0],2);
    memcpy(&sendData[4],&RX_Frame.deviceId[0],4);

    for(k = 2; k < 16; k++)
    {
         parity ^= sendData[k];
    }
  
    RX_Frame.parityBit =  parity;
    printf("SendDataLen : %d\n",SendDataLen);

    memcpy(&sendData[SendDataLen4g-1],&RX_Frame.parityBit,1);
   

    printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  %02x\n",sendData[0],sendData[1],sendData[2],sendData[3],sendData[4],sendData[5],
   sendData[6],sendData[7],sendData[8],sendData[9],sendData[10],sendData[11],sendData[12],sendData[13],sendData[14],sendData[15],sendData[16],sendData[17],sendData[18],sendData[19],sendData[20],sendData[21],sendData[22],sendData[23]
   ,sendData[24],sendData[25],sendData[26],sendData[27],sendData[28],sendData[29],sendData[30],sendData[31]);
   // sendCmdWifi2(sendData);
   sendCmd4g_byte(sendData);

}

int sendToSerStrlen = 0;

void DBC_CanDataPackToSerMsg(SEND_FRAME_TO_SER_MSG *RX_Frame,SIGNAL_t * g_SIGNALDate,int signalFlagNum)
{
    printf("******* signalFlagNum : %d\n",signalFlagNum);
    printf("signal : %d  signal_type : %d signal_identifying : %d signal_strlen  : %d\n",g_SIGNALDate->signal,g_SIGNALDate->signal_type,g_SIGNALDate->signal_identifying,g_SIGNALDate->signal_strlen);

    printf("**************************************************************************\n");
    //������
    RX_Frame->data[signalFlagNum].signal_type = (unsigned char)(g_SIGNALDate->signal_type << 4);
    RX_Frame->data[signalFlagNum].signal_type |= (g_SIGNALDate->Realtime_Show_flag << 3);
    RX_Frame->data[signalFlagNum].signal_type |=  g_SIGNALDate->signal_strlen;

    RX_Frame->data[signalFlagNum].signal_identifying = g_SIGNALDate->signal_identifying;
           
    printf("g_SIGNALDate->signal_type : %d\n",g_SIGNALDate->signal_type);
    printf("g_SIGNALDate->signal : %d\n",g_SIGNALDate->signal);

    printf("DATA  strlen: %d\n",g_SIGNALDate->signal);

    if(g_SIGNALDate->signal_num_flag)
    {
        memcpy(&RX_Frame->data[signalFlagNum].SignalDataDouble,&g_SIGNALDate->SignalDataDouble,2);
        RX_Frame->data[signalFlagNum].signal_num_flag = g_SIGNALDate->signal_num_flag;
        RX_Frame->data[signalFlagNum].sig_strlen = 4;
    }
    else
    {
        memcpy(&RX_Frame->data[signalFlagNum].SignalDateOne,&g_SIGNALDate->SignalDateOne,1);
        RX_Frame->data[signalFlagNum].signal_num_flag = g_SIGNALDate->signal_num_flag;
        RX_Frame->data[signalFlagNum].sig_strlen = 3;
    }       
    printf("RX_Frame.frame_type %02X  %02X  %02X \n",RX_Frame->frame_type[0],RX_Frame->frame_type[1],RX_Frame->data[signalFlagNum].signal_type);    
}


void sendDataToSer()
{
    int i=0;
    if(SignalTypeMAX >= 0)
    {
        presentTime =  GetSystemTimeMs();
        if(presentTime - startTime >= 1000)
        {
            printf("************time1 : %f\n",startTime);
            printf("************time2 : %f\n",presentTime);
            startTime = presentTime;
            for(i = 0;i <= SignalTypeMAX;i++)
            {
                SendCanMsgToServerMsg(DataToSerMsg[SIG_CFG.MapItem[i].Signal_Key], SIG_CFG.MapItem[i].VehicleFlag,SIG_CFG.MapItem[i].Signal_Key); 
                DataToSerMsg[SIG_CFG.MapItem[i].Signal_Key].sendToSerStrlen = 0;
            }
             SignalTypeMAX = -1;

        }

    }
    
}


/* *
 * SIG_GatherToSerMsg()  :   同类型信号打包函数
 * signal_type           ：  信号类型
 * SignalMsg             :   信号数据
 * */

void SIG_GatherToSerMsg(int signal_type,SIGNAL_t * SignalMsg)
{
    printf("**********signal type :%d\n",signal_type);
    if(signal_type >= SignalTypeMAX)
    {
       SignalTypeMAX = signal_type;
    }
    SIG_CFG.MapItem[signal_type].Signal_Key = signal_type;
    DBC_CanDataPackToSerMsg(&DataToSerMsg[SIG_CFG.MapItem[signal_type].Signal_Key],SignalMsg,SIG_CFG.MapItem[signal_type].VehicleFlag);
    if(SIG_CFG.MapItem[signal_type].VehicleFlag == CAN_DATA_SIZE)
    {
        SIG_CFG.MapItem[signal_type].VehicleFlag = 0;
    }
    else
    {
       SIG_CFG.MapItem[signal_type].VehicleFlag++;
    }
}



/*��ͨ��DBC��������������źŷ��뷢�ͽṹ��
int SigIdentifying : �źű�ʶ
int signal_Type   : �ź�����
float sigDate  : �����������
SIGNAL_TYPE_t **SIG_List   ������ýṹ��
*/

void SIG_Gather(int signal_type,SIGNAL_t * SignalMsg)
{
    int i = 0;
    printf("***********signal type : %d\n",signal_type);
    //��ȡ��ǰʱ��
    // presentTime = getTime();
    if(signal_type >= SignalTypeMAX)
    {
       SignalTypeMAX = signal_type;
    }
    printf("test signal: %d\n",SignalMsg->signal);
    
    SIG_CFG.MapItem[signal_type].Signal_Key = signal_type;
    DBC_CanDataPack(&candata[SIG_CFG.MapItem[signal_type].Signal_Key],SignalMsg,SIG_CFG.MapItem[signal_type].VehicleFlag);

    SIG_CFG.MapItem[signal_type].VehicleFlag++;
 
    //presentTime =  GetSystemTimeMss();
    

   // signalNumFlag++;

    for(i=0;i<=SignalTypeMAX;i++)
    {
        if(SIG_CFG.MapItem[i].VehicleFlag == 10)
        {
            SignalStrlen = 4+8+SIG_CFG.MapItem[i].VehicleFlag * 3;
            SendDataLen = SIG_CFG.MapItem[i].VehicleFlag;
            printf("SignalStrlen : %d\n",SignalStrlen);
            printf("SendDataLen : %d\n",SendDataLen);

            SendCanMsgToServer(candata[SIG_CFG.MapItem[i].Signal_Key], SIG_CFG.MapItem[i].VehicleFlag,SIG_CFG.MapItem[i].Signal_Key); 
            SIG_CFG.MapItem[i].VehicleFlag = 0;

        }
        SIG_CFG.MapItem[i].VehicleFlag == 0;
    }
      


    #if 0
    switch (signal_type)
        {
            case 0:
            printf("*********type0***********");
            DBC_CanDataPack(&candata[Motor],&SignalMsg,MotorFlag); 
            MotorFlag++;
            break;
            case 1:
            printf("*********type1***********");
            DBC_CanDataPack(&candata[Motor],&SignalMsg,MotorFlag); 
            MotorFlag++;
            break;
            case 2:
            printf("*********type2***********");
            DBC_CanDataPack(&candata[BMS],&SignalMsg,BMSFlag); 
            BMSFlag++;
            break;
            case 3:
            printf("*********type3***********");
            DBC_CanDataPack(&candata[BMS],&SignalMsg,BMSFlag); 
            BMSFlag++;
            break;
            case 4:
            printf("*********type4***********");
            DBC_CanDataPack(&candata[Vehicle],&SignalMsg,VehicleFlag); 
            VehicleFlag++;
            break;
        
        default:
        printf("no is type\n");
            break;
         }
         #endif
}


char msgdatatoser[2];

#if 1

/*
raw��Ϣ������������frame�е�raw��Ϣ����Ϊphy��Ϣ
����BO_List�е�value��
struct can_frame *frame��Ҫ������frame
BO_Unit_t **BO_List��BO_List�ṹ��
*/

void DBC_Explain(tCAN_RAW_MSG *frame,BO_Unit_t **BO_List)
{
    int i = 0;
    printf("recv msg [%ld](%d) [%d] [%02X %02X %02X %02X %02X %02X %02X %02X]  \n",frame->dwMsgID,frame->ucChan,frame->bExt,frame->ucLen,frame->ucData[0],
    frame->ucData[1],frame->ucData[2],frame->ucData[3],frame->ucData[4],frame->ucData[5],frame->ucData[6],frame->ucData[7]);

    while(BO_List[i] != NULL)
    {
        printf("bolist loop \n ");
        if(frame->dwMsgID == BO_List[i]->message_id) //detect the can id
        {
           // printf(" BO_List[i]->message_id �� %d\n",BO_List[i]->message_id);
            /*ID Mached*/
            int t = 0;
            while(BO_List[i]->SG_List[t] != NULL) /*SG_LOOP*/
            {
                 int tmp = 0;
                 int type = 0;
                 int sig_id = 0;
                 int size=0;
                 int showFlag = 0;
                  memset(&canToSerMeg.SignalDataDouble,0,2);
                   memset(&canToSerMeg.SignalDateOne,0,1);
                if(0 == BO_List[i]->SG_List[t]->Bit_order)   //���ģʽ
                {
                    printf("*************da\n");
                     size = BO_List[i]->SG_List[t]->signal_size;
                    int start = BO_List[i]->SG_List[t]->start_bit;
                    type = BO_List[i]->SG_List[t]->signal_type;
                    sig_id = BO_List[i]->SG_List[t]->signal_identifying;
                    showFlag = BO_List[i]->SG_List[t]->realtime_show_flag;
                    printf("size : %d ; start : %d ; type ; %d \n",size,start,type);

                    tmp = 0;
                    /*Pharse the motorla mode msg!*/
                    int crossBytes = CrossByte(start, size);
                    // printf("crossBytes: %d\n",crossBytes);
                    if((size / 8) > 1)
                    {
                        tmp = (frame->ucData[BO_List[i]->SG_List[t]->start_bit] << 8) | frame->ucData[(BO_List[i]->SG_List[t]->signal_size / 8 + BO_List[i]->SG_List[t]->start_bit) - 1];
                        //printf("frame->ucData : %02X   %02X\n",frame->ucData[BO_List[i]->SG_List[t]->start_bit],frame->ucData[(BO_List[i]->SG_List[t]->signal_size / 8 + BO_List[i]->SG_List[t]->start_bit) - 1]);
                        memcpy(&g_SIGNAL.SignalDataDouble,&frame->ucData[BO_List[i]->SG_List[t]->start_bit],size/8);
                        g_SIGNAL.signal_num_flag = 1;
                       
                       
                    }
                    else
                    {
                        tmp = frame->ucData[BO_List[i]->SG_List[t]->start_bit];
                        memcpy(&g_SIGNAL.SignalDateOne,&frame->ucData[BO_List[i]->SG_List[t]->start_bit],1);
                        g_SIGNAL.signal_num_flag = 0;
                        //strncpy(&canToSerMeg.SignalDateOne,&frame->ucData[BO_List[i]->SG_List[t]->start_bit],1);
                       
                    }
                    
                
#if 0
                    if(crossBytes)/*cross bytes*/
                    {
                        int offset = CrossBytesLocation(start, size, crossBytes);
                        printf("offset: %d\n",offset);
                        if(2 == crossBytes)
                        {
                            tmp = ((((frame.data[start/8])<<8) | (frame.data[start/8+1])) >> (offset)) & bit_mask(size);
                        }
                        if(3 == crossBytes)
                        {
                            tmp =((((frame.data[start/8])<<16) | (frame.data[start/8+1]<<8)|frame.data[start/8+2]) >> (offset)) & bit_mask(size);
                        }
                    }
                    else
                    {
                        tmp = ( (frame.data[start/8] ) >> (start%8 - size +1)) & bit_mask(size);
                    }
                     printf("1>>%lf ",tmp);

                    /*value type_ signed or unsigned*/
                    if(BO_List[i]->SG_List[t]->val_type)
                    {
                        if(8 == size)
                        {
                            tmp = (signed char)tmp;
                            tmp = (double)tmp;
                        }
                        else if(16 == size)
                        {
                            tmp = (short)tmp;
                            //tmp = (double)tmp;
                        }
                        else if(32 == size)
                        {
                            tmp = (float)tmp;
                            //tmp = (double)tmp;
                        }
                         printf("2>>%lf ",tmp);
                    }
 #endif
                    /*Factor and Offset*/
                    printf("facator   offset  : %f   %f\n",BO_List[i]->SG_List[t]->facator,BO_List[i]->SG_List[t]->offset);
                        tmp = tmp * BO_List[i]->SG_List[t]->facator + BO_List[i]->SG_List[t]->offset;
                   
                    /*Max and Min*/
                        if(tmp > BO_List[i]->SG_List[t]->maximum)
                        {
                            tmp = BO_List[i]->SG_List[t]->maximum;
                        }
                        if(tmp < BO_List[i]->SG_List[t]->minimum)
                        {
                            tmp = BO_List[i]->SG_List[t]->minimum;
                        }
                       
                    /*assign value*/
                    BO_List[i]->SG_List[t]->value = tmp;
                    /*Print nuit*/
                    #ifdef SHOW
                    printf(">>%d ",tmp);
                    printf(" %s ", BO_List[i]->SG_List[t]->unit);
                    #endif // SHOW
                }
                else   //С��ģʽ
                {
                    printf("---------short mode\n");
                     size = BO_List[i]->SG_List[t]->signal_size;
                    int start = BO_List[i]->SG_List[t]->start_bit;
                     type = BO_List[i]->SG_List[t]->signal_type;
                     sig_id = BO_List[i]->SG_List[t]->signal_identifying;
                     showFlag = BO_List[i]->SG_List[t]->realtime_show_flag;
                      printf("size : %d ; start : %d ; type ; %d \n",size,start,type);
                   
                     tmp = 0;
                    /*Pharse the motorla mode msg!*/
                    int crossBytes = CrossByte(start, size);
                   // printf("crossBytes: %d\n",crossBytes);
                    if((size / 8) > 1)
                    {
                        tmp = ((frame->ucData[start] & 0xFF) ) | ((frame->ucData[(size / 8 + start) - 1] & 0xFF ) << 8);
                       // printf("frame->ucData : %02X   %02X\n",frame->ucData[start],frame->ucData[(size / 8 + start) - 1]);
                         memcpy(&g_SIGNAL.SignalDataDouble[0],&frame->ucData[(size / 8 + start) - 1],1);
                        memcpy(&g_SIGNAL.SignalDataDouble[1],&frame->ucData[start],1);
                        g_SIGNAL.signal_num_flag = 1;
                    }
                    else
                    {
                        tmp = frame->ucData[BO_List[i]->SG_List[t]->start_bit];
                        memcpy(&g_SIGNAL.SignalDateOne,&frame->ucData[BO_List[i]->SG_List[t]->start_bit],1);
                        g_SIGNAL.signal_num_flag = 0;
                        
                    }
                     /*Factor and Offset*/
                        tmp = tmp * BO_List[i]->SG_List[t]->facator + BO_List[i]->SG_List[t]->offset;
                    /*Max and Min*/
                        if(tmp > BO_List[i]->SG_List[t]->maximum)
                        {
                            tmp = BO_List[i]->SG_List[t]->maximum;
                        }
                        if(tmp < BO_List[i]->SG_List[t]->minimum)
                        {
                            tmp = BO_List[i]->SG_List[t]->minimum;
                        }
                    /*assign value*/
                    BO_List[i]->SG_List[t]->value = tmp;
                    /*Print nuit*/
                    
                    #ifdef SHOW
                    printf(">>%d ",tmp);
                    printf(" %s ", BO_List[i]->SG_List[t]->unit);
                    #endif // SHOW
                    
                }
                
                #ifdef SHOW
                printf("\n");
                #endif // SHOW
              
               
                g_SIGNAL.signal = tmp;
                g_SIGNAL.signal_type = type;
                g_SIGNAL.signal_identifying =  sig_id ;
                g_SIGNAL.Realtime_Show_flag = showFlag;
                g_SIGNAL.signal_strlen = size / 8;


              

               // printf("signal : %d  signal_type : %d signal_identifying : %d signal_strlen  : %d\n",g_SIGNAL.signal,g_SIGNAL.signal_type,g_SIGNAL.signal_identifying,g_SIGNAL.signal_strlen);


                //SIG_Gather(type,&g_SIGNAL);
               // InDbcRawMsgQueue(&g_TxSignalDataQueue,&g_SIGNAL);
                // SetCanRawMsg(&g_SIGNAL);
            
                SIG_GatherToSerMsg(type,&g_SIGNAL);   //信号封装函数
                  t++;
                
            }
        break;
        }
        else
        {
            printf("no id\n");
        }
        i++;
    }
    //printf("Number of BO:%d\n",i);
    
}
#endif