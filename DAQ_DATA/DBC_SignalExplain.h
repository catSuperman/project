#ifndef __DBC_SIGNALEXPLAIN__
#define __DBC_SIGNALEXPLAIN__


#include "BO.h"
#include <pthread.h>
#include "DataTypes.h"
#include "can.h"

#define SIGNAL_NUM 21

#define RAW_MSG_QUEUE_SIZE_dbc 10240

typedef struct
{
    int signal;
    int signal_type;    //�ź�����
    int signal_identifying;          //�źű�ʶ
   
}DATA_REGION;


typedef struct
 {
    DATA_REGION data[50];
    unsigned char frame_head[2] ;   //֡ͷ
    unsigned char frame_type[2];
    unsigned char deviceId[4];
    unsigned char parityBit;
 }SEND_FRAME_MSG;

 SEND_FRAME_MSG candata[10];

typedef struct 
 {
   int signal_type;    //�ź�����
   int signal_identifying;          //�źű�ʶ
   unsigned char SignalDataDouble[2];
   unsigned char SignalDateOne;
   int signal_num_flag;
   int sig_strlen;
   
 }CAN_DATA_TO_SER;

typedef struct
 {
   CAN_DATA_TO_SER data[50];
   unsigned char frame_head[2] ;   //֡ͷ
   unsigned char frame_type[2];
   unsigned char deviceId[4];
   unsigned char parityBit;
   int sendToSerStrlen;
  
 }SEND_FRAME_TO_SER_MSG;

SEND_FRAME_TO_SER_MSG  DataToSerMsg[10];


 CAN_DATA_TO_SER canToSerMeg;
 


//void pharse(struct can_frame *frame,BO_Unit_t **BO_List);
//void pharse2(struct can_frame *frame,BO_Unit_t **BO_List);
void DBC_Explain( tCAN_RAW_MSG * frame,BO_Unit_t **BO_List);
void DataPackaging();
void InitInterpreter();
//#define SHOW
#endif // __PHARSE_H__