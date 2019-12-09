#include <time.h>
#include <pthread.h>
#include "systemtimer.h"

unsigned int g_GlobalErrorCode = 0;

struct timespec g_timer_start;
struct timespec g_timer_curr;

void InitSystemTimer()
{
    clock_gettime(CLOCK_MONOTONIC, &g_timer_start); 
}

double GetSystemTimeMs()
{
    clock_gettime(CLOCK_MONOTONIC, &g_timer_curr); 
    double dbTimestamp = (g_timer_curr.tv_sec - g_timer_start.tv_sec) * 1000 + (g_timer_curr.tv_nsec - g_timer_start.tv_nsec)/1000000;
    return dbTimestamp;

}

void SetError(unsigned char index,boolean bActive)
{
    unsigned int temp = 1;
    temp = temp << index;
    if(bActive)
    {
        g_GlobalErrorCode |= temp;
    }
    else
    {
        g_GlobalErrorCode &= (~temp);
    }
}

boolean IsErrorActive(unsigned char index)
{
    unsigned int temp = 1;
    temp = temp << index;
    if(g_GlobalErrorCode&temp)
    {
        return true;
    }
    return false;
}