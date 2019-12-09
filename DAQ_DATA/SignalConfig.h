#ifndef __SIGNALCONFIG__
#define __SIGNALCONFIG__

#include "DataTypes.h"

boolean SignalBaseConfig();

typedef struct 
{
    int Signal_Key;
    unsigned char Signal_Value[10];
    int VehicleFlag;
}SIG_MAP;

typedef struct 
{
   SIG_MAP MapItem[20];
   unsigned char SignalTypeMap[50];
   unsigned char SignalMapName;
}SIG_CFG_MAP;

SIG_CFG_MAP SIG_CFG;






















#endif // __PHARSE_H__