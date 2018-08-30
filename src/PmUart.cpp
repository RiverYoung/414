/*****************************************************
 Copyright 2003-2017 Hangzhou Hikvision Digital Technology Co., Ltd.
 FileName: PmUart.cpp
 Description: TODO
 Others:
 Author: wulonghua
 Date: 2016-12-28
 Version: V1.0.0
 Modification History:
 *****************************************************/

#include "PmUart.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "mthread.h"
#include "debug.h"
#include "mapi_types.h"
#include "drvUART.h"
#include "drvXC_IOPort.h"
#include "apiXC.h"
#include <new>
#include "McuGetBack.h"
#include "HikLock.h"
#include <sys/wait.h>
#include "CuartControl.h"

unsigned char PmUart::g_exitPmUartStat = 0;
unsigned char PmUart::g_cmdLine[CMD_LINE_LEN] = {0};
unsigned char PmUart::g_mcuPac[CMD_LINE_LEN] = {0};
PmUart* PmUart::g_pInstance = NULL;
unsigned short PmUart::g_recieveBytes = 0;
int PmUart::rlastPacNum = 0;
pthread_mutex_t PmUart::g_singletonMutex = PTHREAD_MUTEX_INITIALIZER;
char PmUart::g_pressflag = 0;

PmUart::PmUart()
{
	memset(&m_pmthread, 0 , sizeof(pthread_t));
	g_exitPmUartStat = 1;
	m_input_fd = -1;

#if GESTRUERECONGNITION
	memset(&m_osdTmpX, 0, sizeof(unsigned long));
	memset(&m_osdTmpY, 0, sizeof(unsigned long));
	memset(&m_atTmpX1, 0, sizeof(unsigned long));
	memset(&m_atTmpY1, 0, sizeof(unsigned long));
	memset(&m_atTmpX2, 0, sizeof(unsigned long));
	memset(&m_atTmpY2, 0, sizeof(unsigned long));
	memset(&m_osdStartTime, 0, sizeof(unsigned long));
	memset(&m_atStartTime, 0, sizeof(unsigned long));
	memset(&m_firstX, 0, sizeof(unsigned long));
	memset(&m_firstY, 0, sizeof(unsigned long));
	memset(&m_coordinate, 0, 4*sizeof(unsigned long));
#endif
}

PmUart::~PmUart()
{
	delete g_pInstance;
	g_pInstance = NULL;
}

PmUart* PmUart::GetInstance(void)
{
	if(NULL == g_pInstance)
	{
		/*thread-safe*/
		HikLock m_hikLock(g_singletonMutex);
		g_pInstance = new (std::nothrow) PmUart;
		assert(g_pInstance);
	}

	return g_pInstance;
}


void* PmUart::thrProcess(void* arg)
{
	assert(arg);
	((PmUart *)arg)->pmRev();
	return NULL;
}

unsigned char PmUart::StartPmThrProcess(void)
{
	unsigned char ret;

	pthread_attr_init(&m_pmattr);
	pthread_attr_setstacksize(&m_pmattr, PTHREAD_STACK_SIZE);
	ret = PTH_RET_CHK(pthread_create(&m_pmthread,  &m_pmattr, thrProcess, this));

	/*0:create mcu return back thread success;1: failed*/
	return (ret != 0) ? 0 : 1;
}

void PmUart::ExitPmThrProcess(void)
{
	if(0 == g_exitPmUartStat)
	{
		g_exitPmUartStat = 1;
		void* thread_result;
		unsigned char ret;
		if(m_pmthread != 0)
		{
			ret = PTH_RET_CHK(pthread_join(m_pmthread, &thread_result));
			if(ret != 0)
				DEBUG("pm thread join failed\n");
			else
			{
				m_pmthread = 0;
				DEBUG("hanging pm thread success\n");
			}
		}
		else
			DEBUG("pm thread has hanged\n");

	}
	else
		DEBUG("pm thread is not running\n");
}

void PmUart::PmWrite(unsigned char *pac, const unsigned int len)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, 0);
	pthread_mutex_init(&m_pmMutex, &attr);
	pthread_mutexattr_destroy(&attr);

	if (m_input_fd == -1)
		usleep(1000*1000);
	assert((m_input_fd >= 0));
	pthread_mutex_lock(&m_pmMutex);
	assert(write(m_input_fd, pac, len) != -1);
	pthread_mutex_unlock(&m_pmMutex);
}

//读取ttys串口终端输入的命令,串口终端连接单片机
void PmUart::pmRev(void)
{
	termios old_options;
	/*listen fd sets*/
	fd_set r_fds;
	static int ret = 0;
	static int len = 0;
	static int pacLen = 0;
	struct timeval timeout;
	//unsigned long sum = 0;
	
	m_input_fd = open(SERIAL1_IN_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY); //O_NOCTTY 如果路径名指向终端设备，不要把这个设备用作控制终端。
									    //O_NONBLOCK    以不可阻断的方式打开文件，也就是无论有无数据读取或等待，都会立即返回进程之中。O_NDELAY  同O_NONBLOCK。
	assert((m_input_fd != -1));

	old_options = pmConfig(&m_input_fd);

	g_exitPmUartStat = 0;

	while(!g_exitPmUartStat)
	{
		/*set Time Out*/
		timeout.tv_sec = 0;
		timeout.tv_usec = 10 * 1000; /* 10 ms*/
		//DEBUG("****running pm recieve thread\n");
		FD_ZERO(&r_fds);
		FD_SET(m_input_fd, &r_fds);
		/* select polling UART, if no data recieve Linux will set Thread sleep. TimeOut time is 10ms*/
		ret = ::select(m_input_fd + 1, &r_fds, NULL, NULL, &timeout); //::调用全局函数，不是自己的成员

		if(ret < 0)
		{
			DEBUG("select polling PmUart failed\n");
		}
		else if(ret == 0)
		{
			//DEBUG("select polling  PmUart timeout\n");
		}
		else if(ret >0 && FD_ISSET(m_input_fd, &r_fds))
		{
			len = read(m_input_fd, &g_cmdLine[g_recieveBytes], CMD_LINE_LEN);

			g_recieveBytes += len;

			/*check out of arry range*/
			assert((g_recieveBytes < CMD_LINE_LEN));


			if(0xAA == g_cmdLine[0])  //0xAA是约定好的串口数据包的起始位
			{
				if(g_recieveBytes < 7)//???
					continue;
				else
				{
					pacLen = g_cmdLine[1] + g_cmdLine[2]*256;  //???

					/*assert(pacLen < CMD_LINE_LEN);*/
					if(pacLen > CMD_LINE_LEN)
					{
						g_recieveBytes = 0;
						memset(g_cmdLine, 0, CMD_LINE_LEN);
						DEBUG("2.pm uart data occur error\n");
					}

					if(g_recieveBytes == pacLen)  //???
					{
						memcpy(g_mcuPac, g_cmdLine, pacLen);
						extractDevPac();
						g_recieveBytes = 0;
						memset(g_cmdLine, 0, CMD_LINE_LEN);	
						memset(g_mcuPac,0, CMD_LINE_LEN);

						#if UART_DEBUG
						DEBUG("1.########pacLen = %d\n",pacLen);
						#endif

					}
					else if(g_recieveBytes > pacLen)
					{
						do 
						{
							DEBUG("1.**************g_recieveBytes = %d\n",g_recieveBytes);
							memcpy(g_mcuPac, g_cmdLine, pacLen);
							g_recieveBytes = g_recieveBytes - pacLen;
							DEBUG("2.**************g_recieveBytes = %d\n",g_recieveBytes);
							memcpy(g_cmdLine, &g_cmdLine[pacLen], g_recieveBytes);
							extractDevPac();
							//memset(&g_cmdLine[g_recieveBytes], 0, (sizeof(g_cmdLine) - g_recieveBytes));
							memset(g_mcuPac,0,sizeof(g_mcuPac));

							#if UART_DEBUG
							DEBUG("2.########pacLen = %d\n",pacLen);
							#endif
						}while (g_recieveBytes >= g_cmdLine[1] + g_cmdLine[2]*256);
					}
					else if(g_recieveBytes < pacLen)
						continue;
				}
			}
			/*data from touchscreen device*/
#if GESTRUERECONGNITION
			else if ((0x1f == g_cmdLine[0])/**&&(0xf7 == g_cmdLine[1])**/)
			{
				if (g_recieveBytes < TOUCHDEVPACLEN)
					continue;
				gestrueProc();

				if (TOUCHDEVPACLEN == g_recieveBytes)
				{
					g_recieveBytes = 0;
					memset(g_cmdLine, 0, CMD_LINE_LEN);
				}
				else if (g_recieveBytes > TOUCHDEVPACLEN)
				{
					g_recieveBytes = g_recieveBytes - TOUCHDEVPACLEN;
					memcpy(g_cmdLine, &g_cmdLine[TOUCHDEVPACLEN], g_recieveBytes);
					memset(&g_cmdLine[g_recieveBytes], 0, CMD_LINE_LEN - g_recieveBytes);
				}
				else
					continue;
			}
#endif
			else
			{
				DEBUG("pm uart data occur error\n");
				g_recieveBytes = 0;
				memset(g_cmdLine, 0, sizeof(g_cmdLine));
			}


		}

	}

	pmConfigReset(&m_input_fd, old_options);
	close(m_input_fd);
	m_input_fd = -1;
}

#if GESTRUERECONGNITION
void PmUart::gestrueProc(void)
{

	if (TOUCHDEVPACLEN == g_cmdLine[2])
	{
		//every times
		MAPI_U32 uart_Xpoint1 = g_cmdLine[7]+g_cmdLine[8]*0xFF;
		MAPI_U32 uart_Ypoint1 = g_cmdLine[9]+g_cmdLine[10]*0xFF;
		MAPI_U32 uart_Xpoint2 = g_cmdLine[17]+g_cmdLine[18]*0xFF;
		MAPI_U32 uart_Ypoint2 = g_cmdLine[19]+g_cmdLine[20]*0xFF;

		if (g_cmdLine[5] != 0x0)
		{
			if (g_pressflag ==  0)
			{
				if ((uart_Ypoint1 >32300)&&(uart_Ypoint1 <= 32767)
					&&(uart_Xpoint1 > 3947)&&(uart_Xpoint1 < 28819))
				{
					m_firstX= uart_Xpoint1;
					m_firstY = uart_Ypoint1;
					m_osdTmpX= uart_Xpoint1;
					m_osdTmpY= uart_Ypoint1;
					m_osdStartTime = mapi_time_utility::GetTime0();
					g_pressflag = 1;                 //OSD GESTRUE
					//DEBUG("######b_pressflag = %d, OSD_time = %ld\n",b_pressflag,OSD_time);
				}
				else if ((g_cmdLine[65] == 0x02)&&(g_cmdLine[15] != 0x0)/**&&(_cmdLine[22] == 0x03)**/)
				{
					m_atTmpX1 = uart_Xpoint1;
					m_atTmpY1 = uart_Ypoint1;
					m_atTmpX2 = uart_Xpoint2;
					m_atTmpY2 = uart_Ypoint2;
					m_atStartTime = mapi_time_utility::GetTime0();
					g_pressflag = 2;			//ASSISTTOUCH GESTRUE
					//DEBUG("######b_pressflag = %d, start_time = %ld\n",b_pressflag,start_time);
				}
			}
			else
			{
				if(g_pressflag == 1)
				{
					//�����׶�
					if((abs(m_osdTmpX- uart_Xpoint1)<300)&&(abs(m_osdTmpY- uart_Ypoint1)<300))
					{
						DEBUG("#### OSD_det_time = %ld\n", mapi_time_utility::GetTime0()-m_osdStartTime);
						if(/**(broad_flag == 0)&&**/(mapi_time_utility::GetTime0()-m_osdStartTime>= 1000))
						{
							VirtualInput::Get_Instance()->Send_Event(139);
							g_pressflag = 0;
						}
					}
					else
					{
						g_pressflag  = 0;
					}
				}
				else if(g_pressflag == 2)
				{
					if((abs(m_atTmpX1 -uart_Xpoint1)<200)&&(abs(m_atTmpY1 -uart_Ypoint1)<200)&&(abs(m_atTmpX2 -uart_Xpoint2)<200)&&(abs(m_atTmpY2 -uart_Ypoint2)<200)
						&&(abs(uart_Xpoint1-uart_Xpoint2)<2000)&&(abs(uart_Ypoint1-uart_Ypoint2)<2000))
					{
						//stop_time = mapi_time_utility::GetTime0();
						//DEBUG("####stop_time = %ld\n",stop_time);
						DEBUG("#### AST_det_time = %ld\n", mapi_time_utility::GetTime0() -m_atStartTime);
						if(/**(broad_flag == 0)&&**/(mapi_time_utility::GetTime0() -m_atStartTime)>=800)
						{
							//SendEventToAndroid(168);
							VirtualInput::Get_Instance()->Send_Event(168);
							m_coordinate[0] = uart_Xpoint1;
							m_coordinate[1] = uart_Ypoint1;
							m_coordinate[2] = uart_Xpoint2;
							m_coordinate[3] = uart_Ypoint2;
							//DEBUG("##########  x1= %ld ,y1= %ld , x2 = %ld , y2 = %ld \n" ,Coordinate[0],Coordinate[1],Coordinate[2],Coordinate[3]);
							g_pressflag = 0;
						}
					}
					else
					{
						g_pressflag = 0;
					}
				}
				else
				{
					g_pressflag = 0;
				}

			}

		}
	}
}

unsigned long* PmUart::GetCoordinate(void)
{
	return m_coordinate;
}
#endif

void PmUart::extractMcuPac(void)
{


}

void PmUart::extractDevPac(void)
{
	int mcuPacLen = 0;
	static int pacNum = 0;
	static int rPacNum1 = 0;
	static int rPacNum2 = 0;

	if(0xAA == g_mcuPac[0])
	{
		mcuPacLen = g_mcuPac[1] + g_mcuPac[2]*256;

		/*IR Touch return*/
		if((0x13 == mcuPacLen)&&(0xfc == g_mcuPac[9]))    //???
		{
			DEBUG("*************IrTouch return back pac\n");
			return;
		}


		if(0x11 == g_mcuPac[3])
		{
			assert((0x02 == g_mcuPac[6]));
			/*data from device*/
			switch(g_mcuPac[5])
			{
				case 0x03:
				{
					/*data from IR*/
				}
				break;
				case 0x04:
				{
					/*data from OPS*/
					DEBUG("mcuPacLen = %d, g_mcuPac[7] = %d.\n", mcuPacLen, g_mcuPac[7]);
				}
				break;
				case 0x05:
				{
					/*data from RB*/
					
				}
				break;
				case 0x06:
				{
					/*data from CC*/
					DEBUG("mcuPacLen = %d, g_mcuPac[7] = %d.\n", mcuPacLen, g_mcuPac[7]);
					if ((0x30 == g_mcuPac[7]) && (0x02 == g_mcuPac[8]))
						CuartControl::GetInstance().CenterControl(g_mcuPac[10]);
						//centerControl(g_mcuPac[10]);
					
				}
				break;
				default:
					break;
			}
		}
		else
		{
			/*data from mcu retrun*/
			if(0 == rlastPacNum)
			{
				pacNum = (mcuPacLen - BACK_HEAD_TAIL)/RETURN_BACK_SIZE;
				rPacNum1 = (mcuPacLen -BACK_HEAD_TAIL)%RETURN_BACK_SIZE;
				rPacNum2 = 0;
			}
			else
			{
				rPacNum2 = RETURN_BACK_SIZE - rlastPacNum;
				pacNum = (mcuPacLen - BACK_HEAD_TAIL - rPacNum2)/RETURN_BACK_SIZE;
				rPacNum1 = (mcuPacLen -BACK_HEAD_TAIL - rPacNum2)%RETURN_BACK_SIZE;
				McuGetBack::GetInstance()->SetBackBuf(&g_mcuPac[BACK_HEAD], rPacNum2);
			}

			DEBUG("**********pacNum = %d,rPacNum1 = %d,rlastPacNum=%d\n",pacNum,rPacNum1,rlastPacNum);

			for(int i = 0; i<pacNum; i++)
			{
				McuGetBack::GetInstance()->SetBackBuf(&g_mcuPac[BACK_HEAD+rPacNum2+i*RETURN_BACK_SIZE], RETURN_BACK_SIZE);
				DEBUG("**********g_mcuPac[n] = %d\n",g_mcuPac[BACK_HEAD+rPacNum2+i*RETURN_BACK_SIZE]);
			}
			if(rPacNum1 > 0)
			{
				rlastPacNum = rPacNum1;
				McuGetBack::GetInstance()->SetBackBuf(&g_mcuPac[BACK_HEAD+rPacNum2+pacNum*RETURN_BACK_SIZE], rPacNum1);
			}
			else
				rlastPacNum = 0;
		}
	}
	else
	{
		DEBUG("mcu pac occur error\n");
	}
}

termios PmUart::pmConfig(int* pfd)
{
	int err = -1;
	struct termios new_options,old_options;

	MDrv_UART_SetIOMapBase();
	assert(mdrv_uart_connect(E_UART_PORT1, E_UART_PIU_UART1) == 0);
	assert(mdrv_uart_open(E_UART_PIU_UART1) == 0);

	/*Set new setting to ttyS1*/
	/*no data recieve,read return 0*/
	err = fcntl(*pfd, F_SETFL, FNDELAY);
	if (-1 == err)
		DEBUG("fcntl failed.\n");
	tcgetattr(*pfd, &old_options);
	/* config ttyS1 not do local ECHO, turn off flow control*/
	tcgetattr(*pfd, &new_options);
	new_options.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOCTL | ECHOPRT | ECHOKE | ISIG);
	new_options.c_iflag  &= ~(INPCK | INLCR | ICRNL | IUCLC | IXON | IXOFF);
	new_options.c_oflag  &= ~OPOST;   /*raw output*/
	speed_t setiospeed=SET_IO_SPEED;
	cfsetispeed(&new_options, setiospeed);
	cfsetospeed(&new_options, setiospeed);
	tcsetattr(*pfd, TCSANOW, &new_options);

	return old_options;
}

void PmUart::pmConfigReset(int* pfd, termios old_options)
{
	tcsetattr(*pfd, TCSANOW, &old_options);

	//mdrv_uart_connect(E_UART_PORT0, E_UART_PIU_UART2);
    mdrv_uart_connect(E_UART_PORT1, E_UART_PIU_UART1);
}

