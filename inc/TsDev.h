/*****************************************************
 Copyright 2003-2017 Hangzhou Hikvision Digital Technology Co., Ltd.
 FileName: TsDev.h
 Description: TODO
 Others:
 Author: wulonghua
 Date: 2016-02-16
 Version: V1.0.0
 Modification History:
 *****************************************************/

#ifndef TSDEV_H_
#define TSDEV_H_
#include "libusb.h"
#include "pthread.h"
#include "mthread.h"

#define X_CONVERT(arg0)       (1920*(arg0)/32767)
#define Y_CONVERT(arg0)       (1080*(arg0)/32767)

#define TOUCHDEV_VENDOR_ID      (0x0483)
#define TOUCHDEV_PRODUCT_ID	    (0xffee)

#define TOUCHDEV_INEP			(0x81)

#define TOUCHDEVPACSIZE         (62)
#define USBTIMEOUT              (1000)
#define MYCONFIG                (0x01)

#define REPORTID				(0x02)

#define ERASER_FLAG 1
#define CVT_TOUCH   1
#define ISOLUTION_TOUCH 2

//((static_cast<float>(x))*2000/32767)
//((static_cast<float>(y))1200/32767)
#define ISOLEUTION_X_CONVERSION(x)  (x)
#define ISOLEUTION_Y_CONVERSION(y)  (y)

#define CVT_X_CONVERSION(x) ((static_cast<float>(x)/32*7.35)*32767/2000)
#define CVT_Y_CONVERSION(y) ((static_cast<float>(y)/32*7.35)*32767/1200)

class TsDev
{
public:
	static unsigned char g_areaflag;
	static TsDev* GetInstance(void);
	unsigned char StartTsDevThrProcess(void);
	void ExitTsDevThrProcess(void);
	unsigned long* GetCoordinate(void);
	unsigned long* GetEraser(void);

	int GetCount(void);
	float GetX(int num);
	float GetY(int num);
	float GetWidth(int num);
	float GetHeight(int num);
	int GetEcoFlag(void);
	void SetEcoFlag(int val);
#if 0
	static void instantiate(void);
#endif

private:
	//static unsigned char g_areaflag;
	static char g_eraserFlag;
	static TsDev* g_pInstance;
	pthread_attr_t m_TsDevAttr;
	pthread_t m_TsDevThread;
	static char g_ExitTsDevUartStat;
	static pthread_mutex_t g_singletonMutex;
	static char g_flag;
	static char g_dev;
	static libusb_device_handle* g_devHandle;
	libusb_context* m_ctx;
	static unsigned char g_recvDat[TOUCHDEVPACSIZE];
	static char g_pressflag;
	static int g_ecoflag;
	static unsigned char g_osdCnt;

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
	unsigned long m_eraser[3];


	TsDev();
	virtual ~TsDev();
	//void test(void);
	int openTsDev(void);
	static void* touchDevThr(void* arg);
	void closeTsDev(void);
	void gestrueProc(void);
	void printDevs(libusb_device** devs);
	void gestrueRecognition(void);
	//void findUSBPath(void);
	//bool copyFileToUSB(unsigned char *fileData);

};
#endif
