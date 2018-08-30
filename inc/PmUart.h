/*****************************************************
 Copyright 2003-2017 Hangzhou Hikvision Digital Technology Co., Ltd.
 FileName: PmUart.h
 Description: TODO
 Others:
 Author: wulonghua
 Date: 2016-12-28
 Version: V1.0.0
 Modification History:
 *****************************************************/

#ifndef PMUART_H_
#define PMUART_H_

#include <termios.h>
#include <pthread.h>
#include "mapi_types.h"
#include "mapi_utility.h"
#include "VirtualInput.h"

/*gestrue recognition by analysis uart data*/
#define GESTRUERECONGNITION            (0)

#define SERIAL1_IN_DEVICE 		"/dev/ttyS1"   //串行端口终端
 /*baudrate = 115200*/
#define SET_IO_SPEED                  (4098)
#define CMD_LINE_LEN                (2048)
#define IR_TOUCH_SIZE                (67)
#define PAC_HEAD_TAIL               (9)
#define PAC_HEAD                         (7)
#define BACK_HEAD_TAIL             (5)
#define BACK_HEAD                      (3)
#define RETURN_BACK_SIZE         (4)

#define TOUCHDEVPACLEN            (67)


#define UART_DEBUG 0

class PmUart
{
public:
	static pthread_mutex_t g_singletonMutex;

	void PmWrite(unsigned char *pac, const unsigned int len);
	static PmUart* GetInstance(void);
	unsigned char StartPmThrProcess(void);
	void ExitPmThrProcess(void);
	MAPI_U32* GetCoordinate(void);
private:
	static unsigned char  g_exitPmUartStat;
	static unsigned char  g_cmdLine[CMD_LINE_LEN];
	static unsigned char  g_mcuPac[CMD_LINE_LEN];
	static unsigned short g_recieveBytes;
	static PmUart* g_pInstance;
	static int rlastPacNum;
	static char g_pressflag;

	pthread_t m_pmthread;
	pthread_attr_t m_pmattr;
	pthread_mutex_t m_pmMutex;
	int m_input_fd;

#if GESTRUERECONGNITION
	unsigned long m_osdTmpX;
	unsigned long m_osdTmpY;
	unsigned long m_atTmpX1;
	unsigned long m_atTmpY1;
	unsigned long m_atTmpX2;
	unsigned long m_atTmpY2;

	unsigned long m_osdStartTime;
	unsigned long m_atStartTime;
	unsigned long m_firstX;
	unsigned long m_firstY;
	unsigned long m_coordinate[4];
#endif

	PmUart();
	~PmUart();
	void extractMcuPac(void);
	void extractDevPac(void);
	termios pmConfig(int* fd);
	void pmConfigReset(int* fd, termios old_options);
	char CheckSumOfarry(char* arry, int lenght);
	void pmRev(void);
	static void* thrProcess(void* arg);

	void gestrueProc(void);
};

#endif /* PMUART_H_ */

