EXEC = daqtest1
OBJS = main.o can.o systemtimer.o rtc.o RawCanMsgLogFile.o  DaqConfig.o DaqService.o gpio.o  wifi_esp8266.o  4g_sim7600.o update.o RecordFile.o BO_Index.o DBC_SignalExplain.o

CROSS = arm-fsl-linux-gnueabi-
CC = $(CROSS)gcc
STRIP = $(CROSS)strip
#CFLAGS = -Wall -g 
#CFLAGS = -g 
all: clean $(EXEC)
$(EXEC):$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lpthread -lrt
	$(STRIP) $@

clean:
	-rm -f $(EXEC) *.o
