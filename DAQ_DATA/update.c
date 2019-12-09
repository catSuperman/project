#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>

#include <signal.h> 
#include <sys/time.h> 
#include "update.h"

void UpdateEdasProgram()
{
	FILE *fp;
	int cnt = 0;
	char buf[200] = {0};
	
	fp = popen("ls /media/sd-mmcblk0p1/boot/ | grep edas_p","r");
	if ( fp != NULL ) 
	{
		cnt = fread(buf, sizeof(char), 200-1, fp);
		if(cnt > 0)
		{
            pclose(fp);
			remove("/root/edas_p");
			sleep(1);
			system("cp /media/sd-mmcblk0p1/boot/edas_p /root/");
			sleep(1);
			remove("/media/sd-mmcblk0p1/boot/edas_p");
			sleep(1);
			system("reboot");
		}
		pclose(fp);
	}
}

void UpdateUsbDeviceProgram()
{
	FILE *fp;
	int cnt = 0;
	char buf[200] = {0};
	
	fp = popen("ls /media/sd-mmcblk0p1/boot/ | grep usb_device","r");
	if ( fp != NULL ) 
	{
		cnt = fread(buf, sizeof(char), 200-1, fp);
		if(cnt > 0)
		{
            pclose(fp);
			remove("/root/usb_device");
			sleep(1);
			system("cp /media/sd-mmcblk0p1/boot/usb_device /root/");
			sleep(1);
			remove("/media/sd-mmcblk0p1/boot/usb_device");
			sleep(1);
			system("reboot");
		}
		pclose(fp);
	}
}


void UpdateProgram()
{
	UpdateEdasProgram();
	UpdateUsbDeviceProgram();
}