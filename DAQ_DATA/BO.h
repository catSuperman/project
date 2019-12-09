#ifndef __BO_H__
#define __BO_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>


#define BOFLAG "BO_ "
#define SGFLAG " SG_ "
//#define DEBUG       //show BO something....
//#define SHOW 
//#define DEBUGLINE
//#define DEBUGPARSE  //start size offset /..debug inf

typedef enum bit_order_t{motorola,intel} bit_order;
typedef enum val_type_t{unsigned_value,signed_value}VALUE_TYPE;

typedef struct SG_{
    char signal_name[50];
    char multiplexer_indicator;
    int start_bit;                  //��ʼbit
    unsigned int signal_size;       //�źų���
    bit_order Bit_order;             //0 :motorola 1:intel
    VALUE_TYPE val_type;             //+:unsigned(0) -:signed(1)
    double facator;
    double offset;
    double minimum;
    double maximum;
    char unit[20];
    char receiver[20];
    struct SG_ *next;               //NOT USED
    double value;
    double value_send;
    int signal_type;                 //�ź�����
    int signal_identifying;          //�źű�ʶ
    int realtime_show_flag;          //ʵʱ��ʾ��ʶ
}SG_t;

//struct of every msg
typedef struct BO_Unit{
    unsigned int message_id;        //msg id 0x123....
    unsigned int message_size;      //DLC MAX 8byte
    char  transmitter[50];
    char  message_name[100];
    SG_t *First_SG;
    struct SG_ *SG_List[50];
}BO_Unit_t;
BO_Unit_t * BO_List[50];        //dbc list




typedef struct {
    int signal;
    int signal_type;                 //�ź�����
    int signal_identifying;          //�źű�ʶ
    int Realtime_Show_flag;          //ʵʱ��ʾ��ʶ
    int signal_strlen;               //�źų���
    unsigned char SignalDataDouble[2];
    unsigned char SignalDateOne;
    int signal_num_flag;                //�ź�������־
}SIGNAL_t;






typedef struct {
    SIGNAL_t SIGNAL_L[50]; 
    pthread_mutex_t m_mutex;

    volatile int m_nCnt;
    volatile int m_nRd;
    volatile int m_nWt;
}SIGNAL_TYPE_QUEUE;






#endif
