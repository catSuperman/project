#include "4g_sim7600.h"


#define DATA_LEN    0xFF

#define B9600 0000015

char tmp_4g[1024];
int fd_4g = 0;
int LTEflag=0;
int beauflag;
int SendDataLen4g = 0;


int openSerial4g(char *cSerialName)
{
    int iFd;

    struct termios opt;
    //打开串口
    iFd = open(cSerialName,  O_RDWR | O_NOCTTY | O_NDELAY);
    if(iFd < 0) {
        perror(cSerialName);
        return -1;
    }

    //获取终端参数
    tcgetattr(iFd, &opt);
#if 0
    beauflag=baud_rate_buf[0];
    //设置波特率
    switch (beauflag)
    {
        case 0:
        cfsetispeed(&opt, B4800);
        cfsetospeed(&opt, B4800);
        break;
        case 1:
        cfsetispeed(&opt, B9600);
        cfsetospeed(&opt, B9600);
        break;
        case 2:
        cfsetispeed(&opt, B19200);
        cfsetospeed(&opt, B19200);
        break;
        case 3:
        cfsetispeed(&opt, B38400);
        cfsetospeed(&opt, B38400);
        break;
        case 4:
        cfsetispeed(&opt, B57600);
        cfsetospeed(&opt, B57600);
        break;
        case 5:
        cfsetispeed(&opt, B115200);
        cfsetospeed(&opt, B115200);
        break;
        case 6:
        cfsetispeed(&opt, B230400);
        cfsetospeed(&opt, B230400);
        break;
        case 7:
        cfsetispeed(&opt, B460800);
        cfsetospeed(&opt, B460800);
        break;
        default:
        break;
    }
#endif
    cfsetispeed(&opt, B115200);
    cfsetospeed(&opt, B115200);




    // 接口属性用termios结构体
    /****************************************
    **struct termios {
    **    tcflag_t  c_cflag/* 控制标志
    **    tcflag_t  c_iflag;/* 输入标志
    **    tcflag_t  c_oflag;/* 输出标志
    **    tcflag_t  c_lflag;/* 本地标志
    **    tcflag_t  c_cc[NCCS];/* 控制字符
    **    };
    ****************************************/

    //c_cflag部分可用选项
    /*****************************************
    * CSTOPB      2位停止位，否则为1位
    * CREAD       启动接收
    * PARENB      进行奇偶校验
    * PARENB      奇校验，否则为偶校验
    * HUPCL       最后关闭时断开
    * CLOCAL      忽略调制调解器状态行
    *****************************************/

    //c_iflag标志可用选项
    /*****************************************
     * INPCK       打开输入奇偶校验
     * IGNPAR      忽略奇偶错字符
     * PARMRK      标记奇偶错
     * ISTRIP      剥除字符第8位
     * IXON        启用/停止输出控制流起作用
     * IXOFF       启用/停止输入控制流起作用
     * IGNBRK      启用/停止输入控制流起作用
     * INLCR       将输入的NL转换为CR
     * IGNCR       忽略CR
     * ICRNL       将输入的CR转换为NL
     * **************************************/

    //c_oflag标志可用选项
    /*******************************************
     * BSDLY       退格延迟屏蔽
     * CMSPAR      标志或空奇偶性
     * CRDLY       CR延迟屏蔽
     * FFDLY       换页延迟屏蔽
     * OCRNL       将输出的CR转换为NL
     * OFDEL       填充符为DEL，否则为NULL
     * OFILL       对于延迟使用填充符
     * OLCUC       将输出的小写字符转换为大写字符
     * ONLCR       将NL转换为CR-NL
     * ONLRET      NL执行CR功能
     * ONOCR       在0列不输出CR
     * OPOST       执行输出处理
     * OXTABS      将制表符扩充为空格
     * *****************************************/

    //c_lflag标志可用选项
    /****************************************
     * ISIG        启用终端产生的信号
     * ICANON      启用规范输入
     * XCASE       规范大/小写表示
     * ECHO        进行回送
     * ECHOE       可见擦除字符
     * ECHOK       回送kill符
     * ECHONL      回送NL
     * NOFLSH      在中断或退出键后禁用刷清
     * IEXTEN      启用扩充的输入字符处理
     * ECHOCTL     回送控制字符为^(char)
     * ECHOPRT     硬拷贝的可见擦除方式
     * ECHOKE      Kill的可见擦除
     * PENDIN      重新打印未决输入
     * TOSTOP      对于后台输出发送SIGTTOU
     * *************************************/

    //c_cc标志
    /*******************************************
     * VINTR       中断
     * VQUIT       退出
     * VERASE      擦除
     * VEOF        行结束
     * VEOL        行结束
     * VMIN        需读取的最小字节数
     * VTIME       与“VMIN”配合使用，是指限定的传输或等待的最长时间
     * *****************************************/

    #if 1
    /*
     * raw mode
     */
    //本地标志
    opt.c_lflag   &=   ~(ECHO   |   ICANON   |   IEXTEN   |   ISIG);
    // 输入标志
    opt.c_iflag   &=   ~(IXON | IXOFF | IXANY);
    // 输出标志
    opt.c_oflag   &=   ~(OPOST);
    // 控制标志
    opt.c_cflag   &=   ~PARENB;
    opt.c_cflag   &=   ~CSTOPB;
    opt.c_cflag   &=   ~CSIZE;
    opt.c_cflag   |=   CS8;

    /*
     * 'DATA_LEN' bytes can be read by serial
     */
    //需读取的最小字节数
    opt.c_cc[VMIN]   =   DATA_LEN;
    //传输或等待的最长时间
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

void sendCmd4g(char *str)
{
    int isNum=0;
    //数据长度
    int txslen = strlen(str);
    //向设备写入数据
    isNum = write(fd_4g,str,txslen);
    if(isNum < 0)
    {
        printf("write data to serial failed! \n");
    }
     else
    {
        //如果写入成功，则返回数据长度
        printf("Tx %d \r\n",isNum);
    }
        //清空数组
        memset(tmp_4g,0,1024);
}
void sendCmd4g_byte(unsigned char *str)
{
    int isNum=0;
    //数据长度
    int txslen = strlen(str);
    //向设备写入数据
    isNum = write(fd_4g,str,SendDataLen4g);
    if(isNum < 0)
    {
        printf("write data to serial failed! \n");
    }
     else
    {
        //如果写入成功，则返回数据长度
        printf("Tx %d \r\n",isNum);
    }
}

char serverip[16]="0"; //每行最大读取字符数
char portnumber[5]="0"; //每行最大读取字符数


void read_server_txt()
{
    FILE *fp;
   int currentLine = 0,i=0;

   char strLine[7]="0"; //每行最大读取字符数
   if((fp=fopen("baudrate.txt","r")) == NULL)
   {
       printf("error\n");
   }
   while(!feof(fp))
   {
       if(currentLine == 16)
       {
           fgets(serverip,16,fp);

           printf("%s\n",serverip);
       }
       if(currentLine == 19)
       {
           fgets(portnumber,5,fp);

           printf("%s\n",portnumber);
       }
       fgets(strLine,7,fp);
       currentLine++;
   }

   fclose(fp);

}


void Set_lte(void)
{
    
    char cmd0[20] = "AT+CIPCLOSE=0\r\n";
    //查询模块注册网
    char cmd1[20] = "AT+CNSMOD?\r\n";
    //查询是否可以进行数据业务
    char cmd2[20] = "AT+CEREG?\r\n";
    //设置为透传模式
    char cmd3[20] = "AT+CIPMODE=1\r\n";
    //打开网络
    char cmd4[20] = "AT+NETOPEN\r\n";
    //获取自身ip地址
    char cmd5[20] = "AT+IPADDR\r\n";
    //ping
    char cmd6[50] = "AT+CIPOPEN=0,\"TCP\",\"139.159.212.103\",8080\r\n";

    char cmd7[50];
    //sprintf(cmd7,"AT+CIPOPEN=0,\"TCP\",\"%s\",%s\r\n",serverip,portnumber);

    //printf("%s\n",cmd7);

    unsigned char ucHeartCounter = 0;
    boolean bMainRunFlag = true;
   // serverip[16];
    //portnumber[5];

      //sendCmd4g(cmd0);
    //  sleep(5);
    //查询模块注册网
   // sendCmd4g(cmd1);
   // sleep(5);
    //查询是否可以进行数据业务
   // sendCmd4g(cmd2);
   // sleep(5);
    //设置为透传模式
    sendCmd4g(cmd3);
    sleep(5);
    //打开网络
    sendCmd4g(cmd4);
    sleep(3);
    //获取自身ip地址
   // sendCmd4g(cmd5);
   // sleep(5);
    //ping
    sendCmd4g(cmd6);
    sleep(3);

    LTEflag=1;

}





