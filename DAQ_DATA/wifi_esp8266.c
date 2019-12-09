#include "wifi_esp8266.h"


#define DATA_LEN                0xFF
//extern int fd_wifi;
int fd_wifi = 0;
char tmp_wifi[1024];
int g_pwififlag=0;

int SendDataLen;

void sendCmdWifi(char *str)
{
    int isNum=0;
    int txslen = strlen(str);
    //向文件描述�?�写入数�?
    isNum = write(fd_wifi,str, txslen);
    if (isNum < 0)
        {
            printf("write data to serial failed! \n");
        }
     else
        {
            printf("Tx %d \r\n",isNum);
        }
        //清空数组
        memset(tmp_wifi,0,1024);
}



void sendCmdWifi2(unsigned char *str)
{
    int isNum=0;
    int txslen = strlen(str);
    //向文件描述�?�写入数�?
    isNum = write(fd_wifi,str, SendDataLen);
    if (isNum < 0)
        {
            printf("write data to serial failed! \n");
        }
     else
        {
            printf("Tx %d \r\n",isNum);
        }
        SendDataLen = 0;
        //清空数组
       // memset(tmp_wifi,0,1024);
}




void Set_Wifi(void)
{
    #if 0
    //设置为AP+Station模式
    char cmd1[20]="AT+CWMODE=3\r\n";
    //重置
    char cmd2[20]="AT+RST\r\n";
    //重置
    char cmd3[20]="AT+CWJAP\r\n";
    //重置
    char cmd4[20]="AT+CWJAP=\"yx\",\"whyx2019\"\r\n";
    //设置为透传
    char cmd5[20]="AT+CIPMODE=1\r\n";
    //TCP通信，�?�置为�?�户�?模式
    char cmd6[50]="AT+CIPSTART=\"TCP\",\"192.168.1.199\",8080\r\n";
    //开始发送数�?
    char cmd7[15]="AT+CIPSEND\r\n";




#endif
    //设置为AP+Station模式
    char cmd1[20]="AT+CWMODE=3\r\n";
    //重置
    char cmd2[20]="AT+RST\r\n";
    //连接ap
    char cmd3[40]="AT+CWSAP=\"ouya\",\"123456789\"\r\n";
    //设置为透传
    char cmd4[20]="AT+CIPMODE=1\r\n";
    //开�?单连�?
    char cmd5[20]="AT+CIPMUX=0\r\n";
    //TCP通信，�?�置为�?�户�?模式
    char cmd6[50]="AT+CIPSTART=\"TCP\",\"192.168.4.2\",8080\r\n";
    //开始发送数�?
    char cmd7[15]="AT+CIPSEND\r\n";

    //开�?服务�?
    char cmd8[15]="AT+CIPSERVER=1,8080\r\n";

   // char data[8]="123456\r\n";

   // unsigned char ucHeartCounter = 0;
   // boolean espFlag = true;
    //设置为AP+Station模式

#if 0
    sendCmdWifi(cmd1);
    sleep(1);
    //重置
   sendCmdWifi(cmd2);
    sleep(2);
    //连接ap
    sendCmdWifi(cmd3);
    sleep(1);
    //设置为透传
    sendCmdWifi(cmd4);
    sleep(1);
#endif

//客户�?模式
 //设置为透传
    sendCmdWifi(cmd4);
    sleep(5);
    sendCmdWifi(cmd5);
    sleep(5);
    //TCP通信，�?�置为�?�户�?模式
    sendCmdWifi(cmd6);
    sleep(5);
    //开始发送数�?
    sendCmdWifi(cmd7);


//服务器模�?

    g_pwififlag=1;



}

int openSerialWifi(char *cSerialName)
{
    int iFd;

    struct termios opt;
    //打开串口
    iFd = open(cSerialName,  O_RDWR | O_NOCTTY | O_NDELAY);
    if(iFd < 0) {
        perror(cSerialName);
        return -1;
    }
    //获取终�??参数
    tcgetattr(iFd, &opt);
    //设置波特�?
#if 0
    switch (beauflag)
    {
        case 1:
        cfsetispeed(&opt, B4800);
        cfsetospeed(&opt, B4800);
        break;
        case 2:
        cfsetispeed(&opt, B9600);
        cfsetospeed(&opt, B9600);
        break;
        case 3:
        cfsetispeed(&opt, B19200);
        cfsetospeed(&opt, B19200);
        break;
        case 4:
        cfsetispeed(&opt, B38400);
        cfsetospeed(&opt, B38400);
        break;
        case 5:
        cfsetispeed(&opt, B57600);
        cfsetospeed(&opt, B57600);
        break;
        case 6:
        cfsetispeed(&opt, B115200);
        cfsetospeed(&opt, B115200);
        break;
        case 7:
        cfsetispeed(&opt, B230400);
        cfsetospeed(&opt, B230400);
        break;
        case 8:
        cfsetispeed(&opt, B460800);
        cfsetospeed(&opt, B460800);
        break;
        default:
        break;
    }
#endif
    cfsetispeed(&opt, B115200);
    cfsetospeed(&opt, B115200);

    printf("uart2 init seccessful \n");


    #if 1
    /*
     * raw mode
     */
    opt.c_lflag   &=   ~(ECHO   |   ICANON   |   IEXTEN   |   ISIG);
    opt.c_iflag   &=   ~(IXON | IXOFF | IXANY);
    opt.c_oflag   &=   ~(OPOST);   //不执行输�?*******************************************
    opt.c_cflag   &=   ~PARENB;    //无�?�偶校验
    opt.c_cflag   &=   ~CSTOPB;    //1位停�?�?
    opt.c_cflag   &=   ~CSIZE;     //先把数据位清�?
    opt.c_cflag   |=   CS8;        //把数�?位�?�置�?8�?

    /*
     * 'DATA_LEN' bytes can be read by serial
     */
    opt.c_cc[VMIN]   =   DATA_LEN;
    opt.c_cc[VTIME]  =   150;
    #endif

    ///////////////////////////////////////
    #if 0

    opt.c_lflag   &=   ~(ICANON | ECHO | ECHOE | ISIG);
    opt.c_iflag   &=   ~(BRKINT   |   ICRNL   |   INPCK   |   ISTRIP   |   IXON);
    //
    opt.c_oflag   &=	~(OPOST);
    opt.c_cflag   &=	~(CSIZE   |   PARENB);
    opt.c_cflag   &=    ~CSIZE;
    opt.c_cflag	  |=	 CS8;	 /* 	* 'DATA_LEN' bytes can be read by serial	 */
    //opt.c_cc[VMIN]	=	DATA_LEN;
    //opt.c_cc[VTIME]	=	150;
    opt.c_cc[VMIN]	=	0;
    opt.c_cc[VTIME]	=	0;
    #endif

    if (tcsetattr(iFd,   TCSANOW,   &opt)<0) {
        return   -1;
    }

    return iFd;
}

