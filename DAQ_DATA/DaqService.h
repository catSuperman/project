#ifndef __DAQ_SERVICE_H_
#define __DAQ_SERVICE_H_

#include "can.h"




boolean IsDaqRecvMsgTimeout();
boolean InitDaqService();
void ExitDaqService();


#endif