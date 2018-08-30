/*****************************************************
 Copyright 2003-2017 Hangzhou Hikvision Digital Technology Co., Ltd.
 FileName: McuGetBack.cpp
 Description: TODO
 Others:
 Author: wulonghua
 Date: 2017-01-07
 Version: V1.0.0
 Modification History:
 *****************************************************/
#include "McuGetBack.h"
#include <new>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "debug.h"
#include "VirtualInput.h"
#include "PmUart.h"
#include "MSrv_Control.h"
#include "Mcu_Upgrade.h"
//#include "HikMediaProc.h"
#include "mapi_interface.h"
#include "drvGPIO.h"
#include "HdmiSrcDet.h"
#include "mapi_audio_customer.h"
#include "TsDev.h"


McuGetBack* McuGetBack::g_pInstance = NULL;
unsigned char McuGetBack::g_mcuBackPac[BACK_PAC_SIZE] = {0};
unsigned char McuGetBack::g_exitBackProcStat = 1;
unsigned char McuGetBack::g_lastVgaStat = 0;
pthread_mutex_t McuGetBack::g_singletonMutex = PTHREAD_MUTEX_INITIALIZER;
volatile u8 McuGetBack::g_OPSStat = 0;
u8 McuGetBack::g_mcuStat = 0;

unsigned char McuGetBack::g_winVirtualKeyPad[19] = {0xAA, 0x13, 0x00, 0x11, 0x00, 0x02, 0x03, 0x1f, 0xf7, 0xfc, 0xad, 0x01, \
	0x02, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00};

unsigned char McuGetBack::g_IsWinVirtualKey[16] = {0xAA, 0x10, 0x00, 0x11, 0x00, 0x02,0x03, 0x55, 0xaa, 0x20, 0x02, 0xaa, 0x00, 0x00, 0xff, 0x00};

unsigned char McuGetBack::g_forbidUsbData[18] = {0xaa, 0x12, 0x00, 0x11, 0x00, 0x02, 0x03, 0x1f, 0xf7, 0xfc, 0x30, \
	0x08, 0x01, 0x00, 0x01, 0x00, 0xff, 0x00};

//isolution touchscreen
unsigned char McuGetBack::g_isolutionData[16] = {0xaa, 0x10, 0x00, 0x11, 0x00, 0x02, 0x03, 0x55, 0xaa, 0x02, 0x02, 0x00, 0x00, 0x03, 0xff, 0x00};

unsigned char McuGetBack::g_forbidUartData[18] = {0xaa, 0x12, 0x00, 0x11, 0x00, 0x02, 0x03, 0x1f, 0xf7, 0xfc, 0x30, \
	0x0a, 0x01, 0x00, 0x01, 0x00, 0xff, 0x00 };

unsigned char McuGetBack::g_startAreaInfo[16] = {0xaa, 0x10, 0x00, 0x11, 0x00, 0x02, 0x03, 0x55, 0xaa, 0x05, 0x02, \
	0x00, 0x00, 0x06, 0xff, 0x00};

unsigned char McuGetBack::g_toggleOPS[9] = {0xaa, 0x09, 0x00, 0x56, 0x01, 0x00, 0x00, 0xff, 0x00};
unsigned char McuGetBack::g_getOPSStat[9] = {0xaa, 0x09, 0x00, 0x58, 0x01, 0x00, 0x00, 0xff, 0x00};

unsigned char McuGetBack::g_toggleAn[9] = {0xaa, 0x09, 0x00, 0x55, 0x01, 0x00, 0x00, 0xff, 0x00};

//extern int gMcuUpgradeState;
bool McuGetBack::g_mUpgradeState = false;
/*C++98 not suport*/
#if 0
std::map<unsigned char, int> McuGetBack::g_Map = new std::map<unsigned char, int>(){  \
	       {IR_KEYCODE_PWR, LINUX_KEYCODE_PWR}, \
		{IR_KEYCODE_MUTE, LINUX_KEYCODE_MUTE}, \
		{IR_KEYCODE_WIN, LINUX_KEYCODE_WIN}, \
		{IR_KEYCODE_SPACE, LINUX_KEYCODE_SPACE}, \
		{IR_KEYCODE_ALT_TAB, LINUX_KEYCODE_ALT_TAB}, \
		{IR_KEYCODE_ALT_F4, LINUX_KEYCODE_ALT_F4}, \
		{IR_KEYCODE_NUM1, LINUX_KEYCODE_NUM1}, \
		{IR_KEYCODE_NUM2, LINUX_KEYCODE_NUM2}, \
		{IR_KEYCODE_NUM3, LINUX_KEYCODE_NUM3}, \
		{IR_KEYCODE_NUM4, LINUX_KEYCODE_NUM4}, \
		{IR_KEYCODE_NUM5, LINUX_KEYCODE_NUM5}, \
		{IR_KEYCODE_NUM6, LINUX_KEYCODE_NUM6}, \
		{IR_KEYCODE_NUM7, LINUX_KEYCODE_NUM7}, \
		{IR_KEYCODE_NUM8, LINUX_KEYCODE_NUM8}, \
		{IR_KEYCODE_NUM9, LINUX_KEYCODE_NUM9}, \
		{IR_KEYCODE_PAINT_BOARD, LINUX_KEYCODE_PAINT_BOARD}, \
		{IR_KEYCODE_NUM0, LINUX_KEYCODE_NUM0}, \
		{IR_KEYCODE_SCREEN_SHOT, LINUX_KEYCODE_SCREEN_SHOT}, \
		{IR_KEYCODE_PAGE_UP, LINUX_KEYCODE_PAGE_UP}, \
		{IR_KEYCODE_SHARE_CLOUD, LINUX_KEYCODE_SHARE_CLOUD}, \
		{IR_KEYCODE_PAGE_DOMN, LINUX_KEYCODE_PAGE_DOWN}, \
		{IR_KEYCODE_RED, LINUX_KEYCODE_RED}, \
		{IR_KEYCODE_GREEN, LINUX_KEYCODE_GREEN}, \
		{IR_KEYCODE_YELLOW, LINUX_KEYCODE_YELLOW}, \
		{IR_KEYCODE_BLUE, LINUX_KEYCODE_BLUE}, \
		{IR_KEYCODE_HOME, LINUX_KEYCODE_HOME}, \
		{IR_KEYCODE_BACKSPACE, LINUX_KEYCODE_BACKSPACE}, \
		{IR_KEYCODE_CUSOR_UP, LINUX_KEYCODE_CUSOR_UP}, \
		{IR_KEYCODE_CUSOR_LEFT, LINUX_KEYCODE_CUSOR_LEFT}, \
		{IR_KEYCODE_CUSOR_RIGHT, LINUX_KEYCODE_CUSOR_RIGHT}, \
		{IR_KEYCODE_CUSOR_DOWN, LINUX_KEYCODE_CUSOR_DOWN}, \
		{IR_KEYCODE_CONFIRM, LINUX_KEYCODE_CONFIRM}, \
		{IR_KEYCODE_MENU, LINUX_KEYCODE_MENU}, \
		{IR_KEYCODE_BACK, LINUX_KEYCODE_BACK}, \
		{IR_KEYCODE_CHANNEL_UP, LINUX_KEYCODE_CHANNEL_UP}, \
		{IR_KEYCODE_CHANNEL_DOWN, LINUX_KEYCODE_CHANNEL_DOWN}, \
		{IR_KEYCODE_ECO, LINUX_KEYCODE_ECO}, \
		{IR_KEYCODE_LOCK, LINUX_KEYCODE_LOCK}, \
		{IR_KEYCODE_VOL_UP, LINUX_KEYCODE_VOL_UP}, \
		{IR_KEYCODE_VOL_DOWN, LINUX_KEYCODE_VOL_DOWN}, \
		{IR_KEYCODE_F1, LINUX_KEYCODE_F1}, \
		{IR_KEYCODE_F2, LINUX_KEYCODE_F2}, \
		{IR_KEYCODE_F3, LINUX_KEYCODE_F3}, \
		{IR_KEYCODE_F4, LINUX_KEYCODE_F4}, \
		{IR_KEYCODE_F5, LINUX_KEYCODE_F5}, \
		{IR_KEYCODE_F6, LINUX_KEYCODE_F6}, \
		{IR_KEYCODE_F7, LINUX_KEYCODE_F7}, \
		{IR_KEYCODE_F8, LINUX_KEYCODE_F8}, \
		{IR_KEYCODE_F9, LINUX_KEYCODE_F9}, \
		{IR_KEYCODE_F10, LINUX_KEYCODE_F10}, \
		{IR_KEYCODE_F11, LINUX_KEYCODE_F11}, \
		{IR_KEYCODE_F12, LINUX_KEYCODE_F12}};
#endif


McuGetBack::McuGetBack()
{
	mMasterVer = 0;
	mSlaveVer = 0;
	mLongPressFlag = 0;
	mLastpadVal = 0;
	memset(&m_keypadTime1, 0, sizeof(m_keypadTime1));
	memset(&m_keypadTime2, 0, sizeof(m_keypadTime2));
	memset(&m_decKeypadTime, 0, sizeof(m_decKeypadTime));
	memset(&m_keypadVal, 0, sizeof(m_keypadVal));
	memset(&mlongpressThr, 0, sizeof(mlongpressThr));
	memset(&m_backpthread, 0, sizeof(m_backpthread));
	memset(&mOpsStatThr, 0, sizeof(mOpsStatThr));
	memset(&mtoggleThr, 0, sizeof(mtoggleThr));
	m_backRingBuf = RingBuffer::Get_Instance()->DSI_RingBuf_Init(BACK_BUF_SIZE, 1);
	g_exitBackProcStat = 1;

	m_Map.insert(std::make_pair(IR_KEYCODE_PWR, LINUX_KEYCODE_PWR));
	m_Map.insert(std::make_pair(IR_KEYCODE_MUTE, LINUX_KEYCODE_MUTE));
	m_Map.insert(std::make_pair(IR_KEYCODE_WIN, LINUX_KEYCODE_WIN));
	m_Map.insert(std::make_pair(IR_KEYCODE_SPACE, LINUX_KEYCODE_SPACE));
	m_Map.insert(std::make_pair(IR_KEYCODE_ALT_TAB, LINUX_KEYCODE_ALT_TAB));
	m_Map.insert(std::make_pair(IR_KEYCODE_ALT_F4, LINUX_KEYCODE_ALT_F4));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM1, LINUX_KEYCODE_NUM1));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM2, LINUX_KEYCODE_NUM2));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM3, LINUX_KEYCODE_NUM3));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM4, LINUX_KEYCODE_NUM4));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM5, LINUX_KEYCODE_NUM5));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM6, LINUX_KEYCODE_NUM6));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM7, LINUX_KEYCODE_NUM7));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM8, LINUX_KEYCODE_NUM8));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM9, LINUX_KEYCODE_NUM9));
	m_Map.insert(std::make_pair(IR_KEYCODE_PAINT_BOARD, LINUX_KEYCODE_PAINT_BOARD));
	m_Map.insert(std::make_pair(IR_KEYCODE_NUM0, LINUX_KEYCODE_NUM0));
	m_Map.insert(std::make_pair(IR_KEYCODE_SCREEN_SHOT, LINUX_KEYCODE_SCREEN_SHOT));
	m_Map.insert(std::make_pair(IR_KEYCODE_PAGE_UP, LINUX_KEYCODE_PAGE_UP));
	m_Map.insert(std::make_pair(IR_KEYCODE_SHARE_CLOUD, LINUX_KEYCODE_SHARE_CLOUD));
	m_Map.insert(std::make_pair(IR_KEYCODE_PAGE_DOMN, LINUX_KEYCODE_PAGE_DOWN));
	m_Map.insert(std::make_pair(IR_KEYCODE_RED, LINUX_KEYCODE_RED));
	m_Map.insert(std::make_pair(IR_KEYCODE_GREEN, LINUX_KEYCODE_GREEN));
	m_Map.insert(std::make_pair(IR_KEYCODE_YELLOW, LINUX_KEYCODE_YELLOW));
	m_Map.insert(std::make_pair(IR_KEYCODE_BLUE, LINUX_KEYCODE_BLUE));
	m_Map.insert(std::make_pair(IR_KEYCODE_HOME, LINUX_KEYCODE_HOME));
	m_Map.insert(std::make_pair(IR_KEYCODE_BACKSPACE, LINUX_KEYCODE_BACKSPACE));
	m_Map.insert(std::make_pair(IR_KEYCODE_CUSOR_UP, LINUX_KEYCODE_CUSOR_UP));
	m_Map.insert(std::make_pair(IR_KEYCODE_CUSOR_LEFT, LINUX_KEYCODE_CUSOR_LEFT));
	m_Map.insert(std::make_pair(IR_KEYCODE_CUSOR_RIGHT, LINUX_KEYCODE_CUSOR_RIGHT));
	m_Map.insert(std::make_pair(IR_KEYCODE_CUSOR_DOWN, LINUX_KEYCODE_CUSOR_DOWN));
	m_Map.insert(std::make_pair(IR_KEYCODE_CONFIRM, LINUX_KEYCODE_CONFIRM));
	m_Map.insert(std::make_pair(IR_KEYCODE_MENU, LINUX_KEYCODE_MENU));
	m_Map.insert(std::make_pair(IR_KEYCODE_BACK, LINUX_KEYCODE_BACK));
	m_Map.insert(std::make_pair(IR_KEYCODE_CHANNEL_UP, LINUX_KEYCODE_CHANNEL_UP));
	m_Map.insert(std::make_pair(IR_KEYCODE_CHANNEL_DOWN, LINUX_KEYCODE_CHANNEL_DOWN));
	m_Map.insert(std::make_pair(IR_KEYCODE_ECO, LINUX_KEYCODE_ECO));
	m_Map.insert(std::make_pair(IR_KEYCODE_LOCK, LINUX_KEYCODE_LOCK));
	m_Map.insert(std::make_pair(IR_KEYCODE_VOL_UP, LINUX_KEYCODE_VOL_UP));
	m_Map.insert(std::make_pair(IR_KEYCODE_VOL_DOWN, LINUX_KEYCODE_VOL_DOWN));
	m_Map.insert(std::make_pair(IR_KEYCODE_F1, LINUX_KEYCODE_F1));
	m_Map.insert(std::make_pair(IR_KEYCODE_F2, LINUX_KEYCODE_F2));
	m_Map.insert(std::make_pair(IR_KEYCODE_F3, LINUX_KEYCODE_F3));
	m_Map.insert(std::make_pair(IR_KEYCODE_F4, LINUX_KEYCODE_F4));
	m_Map.insert(std::make_pair(IR_KEYCODE_F5, LINUX_KEYCODE_F5));
	m_Map.insert(std::make_pair(IR_KEYCODE_F6, LINUX_KEYCODE_F6));
	m_Map.insert(std::make_pair(IR_KEYCODE_F7, LINUX_KEYCODE_F7));
	m_Map.insert(std::make_pair(IR_KEYCODE_F8, LINUX_KEYCODE_F8));
	m_Map.insert(std::make_pair(IR_KEYCODE_F9, LINUX_KEYCODE_F9));
	m_Map.insert(std::make_pair(IR_KEYCODE_F10, LINUX_KEYCODE_F10));
	m_Map.insert(std::make_pair(IR_KEYCODE_F11, LINUX_KEYCODE_F11));
	m_Map.insert(std::make_pair(IR_KEYCODE_F12, LINUX_KEYCODE_F12));

	m_PadMap.insert(std::make_pair(KEYPAD_KEYCODE_1, LINUX_KEYCODE_ECO));
	m_PadMap.insert(std::make_pair(KEYPAD_KEYCODE_2, LINUX_KEYCODE_HOME));
	m_PadMap.insert(std::make_pair(KEYPAD_KEYCODE_3, LINUX_KEYCODE_BACK));
	m_PadMap.insert(std::make_pair(KEYPAD_KEYCODE_4, LINUX_KEYCODE_VOL_DOWN));
	m_PadMap.insert(std::make_pair(KEYPAD_KEYCODE_5, LINUX_KEYCODE_VOL_UP));
	m_PadMap.insert(std::make_pair(KEYPAD_KEYCODE_6, LINUX_KEYCODE_MENU));
	m_PadMap.insert(std::make_pair(KEYPAD_KEYCODE_7, LINUX_KEYCODE_OPS));

	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM0, VIRTUAL_WIN_NUM0));
	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM1, VIRTUAL_WIN_NUM1));
	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM2, VIRTUAL_WIN_NUM2));
	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM3, VIRTUAL_WIN_NUM3));
	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM4, VIRTUAL_WIN_NUM4));
	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM5, VIRTUAL_WIN_NUM5));
	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM6, VIRTUAL_WIN_NUM6));
	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM7, VIRTUAL_WIN_NUM7));
	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM8, VIRTUAL_WIN_NUM8));
	m_winMap.insert(std::make_pair(IR_KEYCODE_NUM9, VIRTUAL_WIN_NUM9));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F1, VIRTUAL_WIN_F1));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F2, VIRTUAL_WIN_F2));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F3, VIRTUAL_WIN_F3));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F4, VIRTUAL_WIN_F4));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F5, VIRTUAL_WIN_F5));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F6, VIRTUAL_WIN_F6));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F7, VIRTUAL_WIN_F7));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F8, VIRTUAL_WIN_F8));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F9, VIRTUAL_WIN_F9));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F10, VIRTUAL_WIN_F10));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F11, VIRTUAL_WIN_F11));
	m_winMap.insert(std::make_pair(IR_KEYCODE_F12, VIRTUAL_WIN_F12));
	m_winMap.insert(std::make_pair(IR_KEYCODE_PAGE_DOMN, VIRTUAL_WIN_PAGEDN));
	m_winMap.insert(std::make_pair(IR_KEYCODE_PAGE_UP, VIRTUAL_WIN_PAGEUP));
	m_winMap.insert(std::make_pair(IR_KEYCODE_CUSOR_DOWN, VIRTUAL_WIN_DOWN));
	m_winMap.insert(std::make_pair(IR_KEYCODE_CUSOR_LEFT, VIRTUAL_WIN_LEFT));
	m_winMap.insert(std::make_pair(IR_KEYCODE_CUSOR_RIGHT, VIRTUAL_WIN_RIGHT));
	m_winMap.insert(std::make_pair(IR_KEYCODE_CUSOR_UP, VIRTUAL_WIN_UP));
	m_winMap.insert(std::make_pair(IR_KEYCODE_BLUE, VIRTUAL_WIN_MOUSE_RIGHT));
	m_winMap.insert(std::make_pair(IR_KEYCODE_BACKSPACE, VIRTUAL_WIN_BACKSPACE));
	m_winMap.insert(std::make_pair(IR_KEYCODE_CONFIRM, VIRTUAL_WIN_ENTER));
	//m_winMap.insert(std::make_pair(IR_KEYCODE_WIN, VIRTUAL_WIN_WIN));
	m_winMap.insert(std::make_pair(IR_KEYCODE_SPACE, VIRTUAL_WIN_SPACE));

	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F1, IS_VIRTUAL_WIN_F1));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F2, IS_VIRTUAL_WIN_F2));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F3, IS_VIRTUAL_WIN_F3));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F4, IS_VIRTUAL_WIN_F4));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F5, IS_VIRTUAL_WIN_F5));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F6, IS_VIRTUAL_WIN_F6));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F7, IS_VIRTUAL_WIN_F7));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F8, IS_VIRTUAL_WIN_F8));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F9, IS_VIRTUAL_WIN_F9));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F10, IS_VIRTUAL_WIN_F10));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F11, IS_VIRTUAL_WIN_F11));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_F12, IS_VIRTUAL_WIN_F12));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_PAGE_DOMN, IS_VIRTUAL_WIN_PAGEDN));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_PAGE_UP, IS_VIRTUAL_WIN_PAGEUP));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_CUSOR_DOWN, IS_VIRTUAL_WIN_DOWN));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_CUSOR_LEFT, IS_VIRTUAL_WIN_LEFT));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_CUSOR_RIGHT, IS_VIRTUAL_WIN_RIGHT));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_CUSOR_UP, IS_VIRTUAL_WIN_UP));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_BLUE, IS_VIRTUAL_WIN_MOUSE_RIGHT));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_BACKSPACE, IS_VIRTUAL_WIN_BACKSPACE));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_CONFIRM, IS_VIRTUAL_WIN_ENTER));
	//m_winMap.insert(std::make_pair(IR_KEYCODE_WIN, VIRTUAL_WIN_WIN));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_SPACE, IS_VIRTUAL_WIN_SPACE));	
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_ALT_TAB, IS_VIRTUAL_ALT_TAB));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_ALT_F4, IS_VIRTUAL_ALT_F4));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_WIN, IS_VIRTUAL_WIN_WIN));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_SCREEN_SHOT, IS_VIRTUAL_WIN_SCREEN));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM1, IS_VIRTUAL_WIN_NUM1));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM2, IS_VIRTUAL_WIN_NUM2));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM3, IS_VIRTUAL_WIN_NUM3));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM4, IS_VIRTUAL_WIN_NUM4));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM5, IS_VIRTUAL_WIN_NUM5));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM6, IS_VIRTUAL_WIN_NUM6));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM7, IS_VIRTUAL_WIN_NUM7));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM8, IS_VIRTUAL_WIN_NUM8));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM9, IS_VIRTUAL_WIN_NUM9));
	m_IswinMap.insert(std::make_pair(IR_KEYCODE_NUM0, IS_VIRTUAL_WIN_NUM0));
}

McuGetBack::~McuGetBack()
{
	RingBuffer::Get_Instance()->DSI_RingBuf_Destroy(m_backRingBuf);
	if (g_pInstance != NULL)
	{
		HikLock m_hikLock(g_singletonMutex);
		delete g_pInstance;
		g_pInstance = NULL;
	}
}


McuGetBack* McuGetBack::GetInstance(void)
{
	if(g_pInstance == NULL)
	{
		/*thread-safe*/
		HikLock m_hikLock(g_singletonMutex);
		g_pInstance = new (std::nothrow)McuGetBack;
		assert(g_pInstance);
	}

	return g_pInstance;
}

void* McuGetBack::mcuGetBackThr(void *arg)
{
	/*arg: pthread_create function's 4th arg*/
	assert(arg);
	((McuGetBack*)arg)->backDataProcess();
	return NULL;
}

unsigned char McuGetBack::StartBackDataProcThr(void)
{
	unsigned char ret;

	pthread_attr_init(&m_backattr);
	pthread_attr_setstacksize(&m_backattr, PTHREAD_STACK_SIZE);
	/*pthread_create function's 3rd arg must be static member*/
	ret = PTH_RET_CHK(pthread_create(&m_backpthread,  &m_backattr, mcuGetBackThr, this));

	/*0:create mcu return back thread success;1: failed*/
	return (0 != ret) ? 0 : 1;

}

void McuGetBack::ExitBackDataProcThr(void)
{
	if(0 == g_exitBackProcStat)
	{
		g_exitBackProcStat = 1;
		void* thread_result;
		unsigned char ret;
		if(m_backpthread != 0)
		{
			ret = PTH_RET_CHK(pthread_join(m_backpthread, &thread_result));
			if(ret != 0)
			{
				DEBUG("mcu return back thread join failed\n");
			}
			else
			{
				m_backpthread = 0;
				DEBUG("hanging mcu return back thread success\n");
			}
		}
		else
		{
			DEBUG("mcu return back thread has hanged\n");
		}

	}
	else
	{
		DEBUG("mcu return back thread is not running\n");
	}
}

void McuGetBack::SetBackBuf(unsigned char *pac, int sz)
{
	int len;
	len = RingBuffer::Get_Instance()->DSI_RingBuf_Write(m_backRingBuf, pac, sz);
	if(len < sz)
	{
		DEBUG("m_backRingBuf freespace is not enough\n");
	}

}

void McuGetBack::backDataProcess(void)
{
	RING_BUFFER* pRingbuffer = NULL;
	pRingbuffer = (RING_BUFFER*)m_backRingBuf;
	g_exitBackProcStat = 0;

	DEBUG("****launch back recieve thread\n");

	while(!g_exitBackProcStat)
	{
		if(pRingbuffer->writeIdx - pRingbuffer->readIdx >= BACK_PAC_SIZE)
		{
			RingBuffer::Get_Instance()->DSI_RingBuf_Read(m_backRingBuf, g_mcuBackPac, BACK_PAC_SIZE);
			switch (g_mcuBackPac[0])
			{
				case 0x10:
					/*return android board send data*/
					break;
				case 0x30:
					/*return version num*/
					DEBUG("MCU Version num high = %d, low = %d.\n", g_mcuBackPac[1], g_mcuBackPac[2]);
					g_mcuStat = 1;
					mMasterVer = static_cast<char>(g_mcuBackPac[1]);
					mSlaveVer = static_cast<char>(g_mcuBackPac[2]);
					break;
				case 0x22:
					/*return airfan level*/
					break;
				case 0x20:
					/*return airfan stat*/
					break;
				case 0x34:
					/*return upgrade stat*/
					break;
				case 0x35:
					/*return mcu program running stat*/
					break;
				case 0x58:
					/*return OPS stat*/
					opsStat(g_mcuBackPac[1], g_mcuBackPac[2]);
					break;
				case 0x59:
					/*return Android stat*/
					androidStat(g_mcuBackPac[1]);
					break;
				case 0x60:
					/*return VGA plug stat*/
					vgaDataProc(g_mcuBackPac[1]);
					break;
				case 0x61:
					/*return keypad value*/
					DEBUG("get key from keypad.\n");
					keypadVal(g_mcuBackPac[1]);
					break;
				case 0x62:
					/*return IR key value*/
					irVal(g_mcuBackPac[1]);
					break;
				case 0x63:
					DEBUG("McuUpgrade :mcu back 111 %d \n", g_mcuBackPac[1]);
					if(1 == g_mcuBackPac[1])
					{
						McuUpgrade::gMcuUpgradeState = PreUpgrade;
					}
					else
					{
						McuUpgrade::gMcuUpgradeState = UpgradeFail;
					}
					break;
				case 0x64:
					DEBUG("McuUpgrade :mcu back 222 %d \n", g_mcuBackPac[1]);
					if(1 == g_mcuBackPac[1])
					{
						McuUpgrade::gMcuUpgradeState = UpgradeStart;
					}
					else
					{
						McuUpgrade::gMcuUpgradeState = UpgradeFail;
					}
					break;
				case 0x45:
					DEBUG("McuUpgrade :mcu back 333 %d \n", g_mcuBackPac[1]);
					if(1 != g_mcuBackPac[1])
					{
						McuUpgrade::gMcuUpgradeState = UpgradeFail;
					}
					break;
				default:
					memset(g_mcuBackPac, 0, BACK_PAC_SIZE);
					break;
			}
		}
		else
		{
			usleep(10*1000);
		}

		/*HP plug in*/
		if(mdrv_gpio_get_level(163))
		 	mapi_audio_customer::GetInstance()->SetSoundMute(SOUND_MUTE_SPEAKER_, E_MUTE_OFF_);
		else
			mapi_audio_customer::GetInstance()->SetSoundMute(SOUND_MUTE_SPEAKER_, E_MUTE_ON_);


	}
}

inline void McuGetBack::vgaDataProc(const unsigned char val)
{
	/*use "xor" function*/
	unsigned char cmd = g_lastVgaStat^val;
	/*1.plug out : cur < last*/
	if(val < g_lastVgaStat)
	{
		switch(cmd)
		{
			case 1:
				/*plug out VGA1*/
				break;
			case 2:
				/*plug out VGA2*/
				break;
			default:
				break;
		}
		//VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_HOME);
	}
	/*2.plug in : cur > last*/
	else if((val > g_lastVgaStat) && (HdmiSrcDet::g_stat == 1))
	{
		switch(cmd)
		{
			case 2:
			{
				/*plug in VGA1*/
				//HikMediaProc::GetInstance()->SetVgaSwitchPort(1);
				VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_HK_VGA1);
//				mdrv_gpio_set_low(122);
				//mapi_interface::Get_mapi_audio()->InputSource_ChangeAudioSource(MAPI_INPUT_SOURCE_SCART);
			}
			break;
			case 1:
			{
				/*plug in VGA2*/
				//HikMediaProc::GetInstance()->SetVgaSwitchPort(2);
				VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_HK_VGA2);
//				mdrv_gpio_set_high(122);
				//mapi_interface::Get_mapi_audio()->InputSource_ChangeAudioSource(MAPI_INPUT_SOURCE_YPBPR2);
			}
			break;
		}
//		if (HdmiSrcDet::g_stat == 1)
//			VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_HK_VGA);
	}
	g_lastVgaStat = val;
}

inline void McuGetBack::opsStat(const unsigned char cmd, const unsigned char stat)
{
	/*pwr stat*/
	if(0x00 == cmd)
	{
		switch(stat)
		{
			case 0x01:
				/*pwr up*/
				break;
			case 0x02:
				/*pwr down*/
				break;
			default:
				break;
		}
	}
	/*os stat*/
	else if(0x01 == cmd)
	{
		switch(stat)
		{
			case 0x01:
				/*os running*/
				DEBUG("OPS is running.\n");
				g_OPSStat = 1;
				break;
			case 0x02:
				/*os down*/
				DEBUG("OPS is shutdown.\n");
				g_OPSStat = 2;
				break;
			default:
				break;
		}
	}

}

void McuGetBack::GetOpsStat(void)
{
	g_OPSStat = 0;
	OPS_STAT_SUM = checkSumOfArry(g_getOPSStat, 8);
	PmUart::GetInstance()->PmWrite(g_getOPSStat, 9);
	DEBUG("send cmd to check ops state.\n");
	//return g_OPSStat;
}

void McuGetBack::startGetOpsStatProc(void)
{
	int err = -1;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE);
	err = pthread_create(&mOpsStatThr, &attr, getOpsStatProc, this);
	if (-1 == err)
		DEBUG("create getopsstate thread failed.\n");
}

void McuGetBack::exitGetOpsStatProc(void)
{
	//void* thr_ret;
	if (mOpsStatThr != 0)
		pthread_detach(mOpsStatThr);

	mOpsStatThr = 0;
}

void* McuGetBack::getOpsStatProc(void* arg)
{
	assert(arg);
	((McuGetBack*)arg)->GetOpsStat();
	return NULL;
}

u8 McuGetBack::AppGetOpsStat(void)
{
	/*child thread g_OPSStat = 0, not Update to main thread.*/
	g_OPSStat = 0; //is important
	//exitGetOpsStatProc();
	startGetOpsStatProc();
	int cnt = 0;
	while (0 == g_OPSStat)
	{
		DEBUG("wait to get ops stat.\n");
		usleep(50*1000);
		/*revert to died circulation*/
		cnt++;
		if (cnt > 10)
		{
			DEBUG("get ops state time out, cnt = %d.\n", cnt);
			cnt = 0;
			return 0;
		}		
	}
	DEBUG("get ops state cnt = %d.\n", cnt);
	exitGetOpsStatProc();

	return g_OPSStat;
}


inline void McuGetBack::androidStat(const unsigned char stat)
{
	switch(stat)
	{
		case 0x01:
			/*normal mode*/
			break;
		case 0x02:
			/*stand-by mode*/
			break;
		default:
			break;
	}
}

#if 0
inline void McuGetBack::keypadVal(const unsigned char val)
{
	DEBUG("**********KeypadVal = %d\n",val);
	/*key down*/
	if(0 != val)
	{
		clock_gettime(CLOCK_MONOTONIC, &m_keypadTime1);
		m_keypadVal = val;
	}
	/*key up*/
	else
	{
		clock_gettime(CLOCK_MONOTONIC, &m_keypadTime2);
		m_decKeypadTime.tv_sec = m_keypadTime2.tv_sec - m_keypadTime1.tv_sec;

		DEBUG("************m_keypadVal = %d,m_decKeypadTime.tv_sec = %ld\n",m_keypadVal, m_decKeypadTime.tv_sec);

		if((m_decKeypadTime.tv_sec > 1)&&(KEYPAD_KEYCODE_1 == m_keypadVal))
			VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_PWR);
		else
		{
			std::map<unsigned char, int>::iterator it = m_PadMap.find(m_keypadVal);
			if(it != m_PadMap.end())
			{
				/*toggle OPS*/
				#if 0
				if (KEYPAD_KEYCODE_7 == m_keypadVal)
				{
					GetOpsStat();
					ExitToggleOPS();
					StartToggleOPS();
				}
				else
				#endif
				{
					DEBUG("send linux keycode = %d\n", m_PadMap[m_keypadVal]);
					VirtualInput::GetInstance()->SendEvent(m_PadMap[m_keypadVal]);
				}
			}
		}

		m_decKeypadTime.tv_sec = 0;
	}
}
#endif

void McuGetBack::startlongpressThr(void)
{
	int err = -1;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE);
	err = pthread_create(&mlongpressThr, &attr, longpressProc, this);
	if (err == -1)
		DEBUG("create longpress thread failed.\n");
}

void McuGetBack::exitlongpressThr(void)
{
	//void* thr_ret;
	if (mlongpressThr != 0)
		pthread_detach(mlongpressThr);

	mtoggleThr = 0;
}


void* McuGetBack::longpressProc(void* arg)
{
	assert(arg);
	((McuGetBack*)arg)->longpressToDo();
	return NULL;
}

void McuGetBack::longpressToDo(void)
{
	long dec = 1500;
	/*key down*/
	while (m_keypadVal)
	{
		clock_gettime(CLOCK_MONOTONIC, &m_keypadTime2);
		m_decKeypadTime.tv_sec = m_keypadTime2.tv_sec - m_keypadTime1.tv_sec;
		m_decKeypadTime.tv_nsec = m_keypadTime2.tv_sec - m_keypadTime1.tv_nsec;
		if (m_decKeypadTime.tv_sec * 1000 + m_decKeypadTime.tv_nsec/1000/1000 > dec)
		{
			m_keypadTime1 = m_keypadTime2;
			mLongPressFlag = 1;
			dec = 50;
			/*longpress*/
			switch (m_keypadVal)
			{
				case KEYPAD_KEYCODE_1:
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_PWR);
					break;
				case KEYPAD_KEYCODE_4:
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_DOWN);
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_DOWN);
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_DOWN);
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_DOWN);
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_DOWN);
					break;
				case KEYPAD_KEYCODE_5:
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_UP);
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_UP);
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_UP);
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_UP);
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_VOL_UP);
					break;
				case KEYPAD_KEYCODE_6:
					VirtualInput::GetInstance()->SendEvent(LINUX_KEYCODE_LOCK);
					m_keypadVal = 0x0;
					break;
				default:
					m_keypadVal = 0x0;
					break;
			}
		}
	}
}


inline void McuGetBack::keypadVal(unsigned char val)
{
	clock_gettime(CLOCK_MONOTONIC, &m_keypadTime1);
	m_keypadVal = val;
	if (val != 0)
	{
		mLastpadVal = val;
		//exitlongpressThr();
		startlongpressThr();
	}
		
	DEBUG("keyPadVal = %d\n", val);

	/*key up*/
	if (0 == val)
	{
		DEBUG("mLongPressFlag = %d.\n", mLongPressFlag);
		/*shortpress*/
		if (0 == mLongPressFlag)
		{
			std::map<unsigned char, int>::iterator it = m_PadMap.find(mLastpadVal);
			if(it != m_PadMap.end())
			{

				DEBUG("send linux keycode = %d\n", m_PadMap[mLastpadVal]);
				VirtualInput::GetInstance()->SendEvent(m_PadMap[mLastpadVal]);

			}
		}
		else
			mLongPressFlag = 0;

		exitlongpressThr();
	}

}

void McuGetBack::StartToggleOPS(void)
{
	int err = -1;
	pthread_attr_t toggleAttr;
	pthread_attr_init(&toggleAttr);
	pthread_attr_setstacksize(&toggleAttr, PTHREAD_STACK_SIZE);
	err = pthread_create(&mtoggleThr, &toggleAttr, toggleOPS, this);
	if (err == -1)
		DEBUG("create toogleOPS thread failed.\n");
}

void McuGetBack::ExitToggleOPS(void)
{
	void* thr_ret;
	if (mtoggleThr != 0)
		pthread_join(mtoggleThr, &thr_ret);

	mtoggleThr = 0;
}

void* McuGetBack::toggleOPS(void* arg)
{
	assert(arg);
	((McuGetBack*) arg)->toggleOPSProc();
	return NULL;
}

void McuGetBack::toggleOPSProc(void)
{
	while (g_OPSStat == 0)
		usleep(50*1000);
	switch (g_OPSStat)
	{
		case 1:
		{
			DEBUG("toggle OPS down.\n");
			TOGGLE_OPS_CMD = 0x01;
			TOGGLE_OPS_SUM = checkSumOfArry(g_toggleOPS, 8);
			//g_OPSStat = 0;
		}
		break;
		case 2:
		{
			DEBUG("toggle OPS start.\n");
			TOGGLE_OPS_CMD = 0x00;
			TOGGLE_OPS_SUM = checkSumOfArry(g_toggleOPS, 8);
			//g_OPSStat = 0;
		}
		break;
		default:
			break;
	}
	g_OPSStat = 0;
	PmUart::GetInstance()->PmWrite(g_toggleOPS, 9);
}

void McuGetBack::AppStartOPS(void)
{
	TOGGLE_OPS_CMD = 0x00;
	TOGGLE_OPS_SUM = checkSumOfArry(g_toggleOPS, 8);
	PmUart::GetInstance()->PmWrite(g_toggleOPS, 9);
}

void McuGetBack::AppShutDownOPS(void)
{
	TOGGLE_OPS_CMD = 0x01;
	TOGGLE_OPS_SUM = checkSumOfArry(g_toggleOPS, 8);
	PmUart::GetInstance()->PmWrite(g_toggleOPS, 9);
}

void McuGetBack::AppShutDownAndroid(void)
{
	TOGGLE_AN_CMD = 0x01;
	TOGGLE_AN_SUM = checkSumOfArry(g_toggleAn, 8);
	PmUart::GetInstance()->PmWrite(g_toggleAn, 9);
}

inline void McuGetBack::irVal(const unsigned char val)
{
	DEBUG("**********irVal = %d\n",val);
	//android,atv,dtv channel
	if (MAPI_INPUT_SOURCE_STORAGE == MSrv_Control::GetInstance()->GetCurrentInputSource()
		|| MAPI_INPUT_SOURCE_NONE == MSrv_Control::GetInstance()->GetCurrentInputSource()
		|| MAPI_INPUT_SOURCE_ATV == MSrv_Control::GetInstance()->GetCurrentInputSource()
		|| MAPI_INPUT_SOURCE_DTV == MSrv_Control::GetInstance()->GetCurrentInputSource())
	{
		std::map<unsigned char, int>::iterator it = m_Map.find(val);
		if (it != m_Map.end())
		{
			DEBUG("send linux keycode = %d\n", m_Map[val]);
			VirtualInput::GetInstance()->SendEvent(m_Map[val]);
		}
	}
	//hdmi,vga channel, and send some event to android os, eg. cusor key
	else
	{
		DEBUG("g_areaflag is %d.\n", TsDev::g_areaflag);
		//cvte touchscreen
		if (TsDev::g_areaflag == CVT_TOUCH)
		{
			std::map<unsigned char, int>::iterator winIt = m_winMap.find(val);
			if (winIt != m_winMap.end())
			{
				winVirtualCheck(0, m_winMap[val]);
				PmUart::GetInstance()->PmWrite(g_winVirtualKeyPad, sizeof(g_winVirtualKeyPad));
				switch (val)
				{
					case IR_KEYCODE_CUSOR_DOWN:
					case IR_KEYCODE_CUSOR_LEFT:
					case IR_KEYCODE_CUSOR_RIGHT:
					case IR_KEYCODE_CUSOR_UP:
					case IR_KEYCODE_CONFIRM:
						VirtualInput::GetInstance()->SendEvent(m_Map[val]);
					break;
					default:
						break;
				}
			}
			else
			{
				switch (val)
				{
					case IR_KEYCODE_WIN:
					{
						winVirtualCheck(VIRTUAL_WIN_WIN, 0);
						PmUart::GetInstance()->PmWrite(g_winVirtualKeyPad, sizeof(g_winVirtualKeyPad));
					}
					break;
					case IR_KEYCODE_ALT_TAB:
					{
						winVirtualCheck(VIRTUAL_WIN_ALT, VIRTUAL_WIN_TAB);
						PmUart::GetInstance()->PmWrite(g_winVirtualKeyPad, sizeof(g_winVirtualKeyPad));
					}
					break;
					case IR_KEYCODE_ALT_F4:
					{
						winVirtualCheck(VIRTUAL_WIN_ALT, VIRTUAL_WIN_F4);
						PmUart::GetInstance()->PmWrite(g_winVirtualKeyPad, sizeof(g_winVirtualKeyPad));
					}
					break;
					case IR_KEYCODE_SCREEN_SHOT:
					{
						winVirtualCheck(0x05, 0x46);
						PmUart::GetInstance()->PmWrite(g_winVirtualKeyPad, sizeof(g_winVirtualKeyPad));
					}
					break;
					default:
					{
						std::map<unsigned char, int>::iterator anIt = m_Map.find(val);
						if (anIt != m_Map.end())
						{
							DEBUG("send linux keycode = %d\n", m_Map[val]);
							VirtualInput::GetInstance()->SendEvent(m_Map[val]);
						}
					}
					break;

				}
			}
		}
		else
		{
			//islotion touchscreen
			std::map<unsigned char, int>::iterator isWinIt = m_IswinMap.find(val);
			if (isWinIt != m_IswinMap.end())
			{
				isWinVirtualCheck(m_IswinMap[val]);
				PmUart::GetInstance()->PmWrite(g_IsWinVirtualKey, sizeof(g_IsWinVirtualKey));

				switch (val)
				{
					case IR_KEYCODE_CUSOR_DOWN:
					case IR_KEYCODE_CUSOR_LEFT:
					case IR_KEYCODE_CUSOR_RIGHT:
					case IR_KEYCODE_CUSOR_UP:
					case IR_KEYCODE_CONFIRM:
						VirtualInput::GetInstance()->SendEvent(m_Map[val]);
					break;
					default:
					break;
				}
			}
			else
			{
				std::map<unsigned char, int>::iterator anIt = m_Map.find(val);
				if (anIt != m_Map.end())
				{
					DEBUG("send linux keycode = %d\n", m_Map[val]);
					VirtualInput::GetInstance()->SendEvent(m_Map[val]);
				}
			}
		}
	}

}

/*0:禁止， 1：开启*/
void McuGetBack::ForbiddenUsbData(unsigned char toggle)
{
	setUsbDataToggle(toggle);
	PmUart::GetInstance()->PmWrite(g_forbidUsbData, sizeof(g_forbidUsbData));
	PmUart::GetInstance()->PmWrite(g_isolutionData, sizeof(g_isolutionData));
}

void McuGetBack::ForbiddenUartData(unsigned char toggle)
{
	setUartDataToggle(toggle);
	PmUart::GetInstance()->PmWrite(g_forbidUartData, sizeof(g_forbidUartData));
	PmUart::GetInstance()->PmWrite(g_isolutionData, sizeof(g_isolutionData));
}

void McuGetBack::ForbiddenTouch(unsigned char toggle)
{
	//if ( MAPI_INPUT_SOURCE_STORAGE == MSrv_Control::GetInstance()->GetCurrentInputSource())
		ForbiddenUartData(toggle);
	//else
		ForbiddenUsbData(toggle);
}

void McuGetBack::winVirtualCheck(unsigned char dat1, unsigned char dat2)
{
  CVT_COMBINATION_KEY = dat1;
  CVT_NORMAL_KEY = dat2;
  CVT_SUM = (checkSumOfArry(&g_winVirtualKeyPad[7], 9))&0xff;
  CVT_CHECKSUM = (checkSumOfArry(&g_winVirtualKeyPad[0],18))&0xff;
}

void McuGetBack::isWinVirtualCheck(unsigned char dat1)
{
  IS_IR_KEY = dat1;
  IS_IR_SUM = (checkSumOfArry(&g_IsWinVirtualKey[7], 6))&0xff;
  IS_IR_CHECKSUM = (checkSumOfArry(&g_IsWinVirtualKey[0],15))&0xff;
}


/*open isolution touchscreen area infomation function*/
void McuGetBack::StartAreaInfo(void)
{
	ISOLUTION_SUM = (checkSumOfArry(&g_startAreaInfo[0], 15))&0xff;
	PmUart::GetInstance()->PmWrite(g_startAreaInfo, sizeof(g_startAreaInfo));
}

void McuGetBack::GetMcuVersion(char* mcuVer)
{
	//DEBUG("mucVer address is %p.\n", mcuVer);
	int cnt = 0;
	while (!g_mcuStat)
	{
		usleep(50*1000);
		cnt++;
		if (cnt > 10)
			return;
	}
	*mcuVer = mMasterVer;
	*(mcuVer + 1) = mSlaveVer;
}

