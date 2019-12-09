#ifndef __GPIO_H_
#define __GPIO_H_

#include "DataTypes.h"

#define LED_NUM    5
#define LED_INDEX_USB   0
#define LED_INDEX_CANRX 1
#define LED_INDEX_CANTX 2
#define LED_INDEX_KLINE 3
#define LED_INDEX_ERR   4

#define LED_BLINK_KEEP_COUNTER  0xFF


boolean InitGpio();
void ExitGpio();

void SetLedQucikBlink(unsigned char LedIndex,unsigned char BlinkCounter);
void SetLedSlowBlink(unsigned char LedIndex,unsigned char BlinkCounter);
void SetLedNormalOff(unsigned char LedIndex);
void SetLedNormalOn(unsigned char LedIndex);
void SetLedNormalAllOff();
void SetLedNormalAllOn();

boolean GetIsT15On();
boolean GetIsGpioInitOK();

boolean Set3GModelPowerOn();
boolean Set3GModelPowerOff();
boolean SetEdasPowerOn();
boolean SetEdasPowerOff();




#endif