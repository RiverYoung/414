/*****************************************************
 Copyright 2003-2017 Hangzhou Hikvision Digital Technology Co., Ltd.
 FileName: McuGetBack.h
 Description: TODO
 Others:
 Author: wulonghua
 Date: 2017-01-07
 Version: V1.0.0
 Modification History:
 *****************************************************/
 #ifndef MCUGETBACK_H_
 #define MCUGETBACK_H_

 #include "RingBuffer.h"
 #include "mthread.h"
 #include <map>
 #include <new>
 #include <time.h>
 #include <stdio.h>
 #include "HikLock.h"

 #define BACK_BUF_SIZE                  (1024)
 #define BACK_PAC_SIZE                  (4)
 #define IR_KEYCODE_PWR               (0x0B)
 #define IR_KEYCODE_MUTE             (0x0F)
 #define IR_KEYCODE_WIN      	        (0x18)
 #define IR_KEYCODE_SPACE            (0x19)
 #define IR_KEYCODE_ALT_TAB        (0x1A)
 #define IR_KEYCODE_ALT_F4          (0x1B)
 #define IR_KEYCODE_NUM1             (0x01)
 #define IR_KEYCODE_NUM2		 (0x02)
 #define IR_KEYCODE_NUM3		 (0x03)
 #define IR_KEYCODE_NUM4             (0x04)
 #define IR_KEYCODE_NUM5             (0x05)
 #define IR_KEYCODE_NUM6		 (0x06)
 #define IR_KEYCODE_NUM7             (0x07)
 #define IR_KEYCODE_NUM8             (0x08)
 #define IR_KEYCODE_NUM9             (0x09)
 #define IR_KEYCODE_PAINT_BOARD             (0x16)
 #define IR_KEYCODE_NUM0             (0x00)
 #define IR_KEYCODE_SCREEN_SHOT             (0x0E)
 #define IR_KEYCODE_PAGE_UP       (0x20)
 #define IR_KEYCODE_SHARE_CLOUD             (0x21)
 #define IR_KEYCODE_PAGE_DOMN   (0x22)
 #define IR_KEYCODE_RED                (0x29)
 #define IR_KEYCODE_GREEN            (0x27)
 #define IR_KEYCODE_YELLOW          (0x28)
 #define IR_KEYCODE_BLUE               (0x26)
 #define IR_KEYCODE_HOME              (0x17)
 #define IR_KEYCODE_BACKSPACE     (0x1E)
 #define IR_KEYCODE_CUSOR_UP       (0x2B)
 #define IR_KEYCODE_CUSOR_LEFT   (0x2D)
 #define IR_KEYCODE_CUSOR_RIGHT (0x2E)
 #define IR_KEYCODE_CUSOR_DOWN (0x2C)
 #define IR_KEYCODE_CONFIRM         (0x2F)
 #define IR_KEYCODE_MENU               (0x15)
 #define IR_KEYCODE_BACK               (0x30)
 #define IR_KEYCODE_CHANNEL_UP     (0x13)
 #define IR_KEYCODE_CHANNEL_DOWN             (0x12)
 #define IR_KEYCODE_ECO                 (0x0A)
 #define IR_KEYCODE_LOCK               (0x14)
 #define IR_KEYCODE_VOL_UP           (0x11)
 #define IR_KEYCODE_VOL_DOWN      (0x10)
 #define IR_KEYCODE_F1                    (0x0C)
 #define IR_KEYCODE_F2                    (0x0D)
 #define IR_KEYCODE_F3                    (0x2A)
 #define IR_KEYCODE_F4                    (0x1C)
 #define IR_KEYCODE_F5                    (0x1D)
 #define IR_KEYCODE_F6                    (0x1F)
 #define IR_KEYCODE_F7                    (0x23)
 #define IR_KEYCODE_F8                    (0x24)
 #define IR_KEYCODE_F9                    (0x25)
 #define IR_KEYCODE_F10                  (0x31)
 #define IR_KEYCODE_F11                  (0x32)
 #define IR_KEYCODE_F12                  (0x33)

 #define KEYPAD_KEYCODE_1		  (0x01)
 #define KEYPAD_KEYCODE_2		  (0x02)
 #define KEYPAD_KEYCODE_3		  (0x04)
 #define KEYPAD_KEYCODE_4         (0x08)
 #define KEYPAD_KEYCODE_5  		  (0x10)
 #define KEYPAD_KEYCODE_6		  (0x20)
 #define KEYPAD_KEYCODE_7		  (0x40)

 #define LINUX_KEYCODE_PWR           (116)
 #define LINUX_KEYCODE_MUTE         (113)
 #define LINUX_KEYCODE_WIN           (82)
 #define LINUX_KEYCODE_SPACE        (57)
 #define LINUX_KEYCODE_ALT_TAB    (79)
 #define LINUX_KEYCODE_ALT_F4      (80)
 #define LINUX_KEYCODE_NUM1         (2)
 #define LINUX_KEYCODE_NUM2         (3)
 #define LINUX_KEYCODE_NUM3         (4)
 #define LINUX_KEYCODE_NUM4         (5)
 #define LINUX_KEYCODE_NUM5         (6)
 #define LINUX_KEYCODE_NUM6         (7)
 #define LINUX_KEYCODE_NUM7         (8)
 #define LINUX_KEYCODE_NUM8         (9)
 #define LINUX_KEYCODE_NUM9         (10)
 #define LINUX_KEYCODE_PAINT_BOARD          (81)
 #define LINUX_KEYCODE_NUM0         (11)
 #define LINUX_KEYCODE_SCREEN_SHOT          (75)
 #define LINUX_KEYCODE_PAGE_UP    (104)
 #define LINUX_KEYCODE_SHARE_CLOUD          (76)
 #define LINUX_KEYCODE_PAGE_DOWN             (109)
 #define LINUX_KEYCODE_RED             (398)
 #define LINUX_KEYCODE_GREEN         (399)
 #define LINUX_KEYCODE_YELLOW       (400)
 #define LINUX_KEYCODE_BLUE            (401)
 #define LINUX_KEYCODE_HOME           (102)
 #define LINUX_KEYCODE_BACKSPACE  (77)
 #define LINUX_KEYCODE_CUSOR_UP    (103)
 #define LINUX_KEYCODE_CUSOR_LEFT (105)
 #define LINUX_KEYCODE_CUSOR_RIGHT           (106)
 #define LINUX_KEYCODE_CUSOR_DOWN            (108)
 #define LINUX_KEYCODE_CONFIRM       (28)
 #define LINUX_KEYCODE_MENU            (139)
 #define LINUX_KEYCODE_BACK             (158)
 #define LINUX_KEYCODE_CHANNEL_UP             (402)
 #define LINUX_KEYCODE_CHANNEL_DOWN        (403)
 #define LINUX_KEYCODE_ECO                            (71)
 #define LINUX_KEYCODE_LOCK                          (72)
 #define LINUX_KEYCODE_VOL_UP                      (115)
 #define LINUX_KEYCODE_VOL_DOWN                 (114)
 #define LINUX_KEYCODE_F1                              (59)
 #define LINUX_KEYCODE_F2                              (60)
 #define LINUX_KEYCODE_F3                              (61)
 #define LINUX_KEYCODE_F4                              (62)
 #define LINUX_KEYCODE_F5                              (63)
 #define LINUX_KEYCODE_F6                              (64)
 #define LINUX_KEYCODE_F7                              (65)
 #define LINUX_KEYCODE_F8                              (66)
 #define LINUX_KEYCODE_F9                              (67)
 #define LINUX_KEYCODE_F10                            (68)
 #define LINUX_KEYCODE_F11                            (87)
 #define LINUX_KEYCODE_F12                            (88)
 #define LINUX_KEYCODE_HK_VGA1			(500)
 #define LINUX_KEYCODE_OPS				(501)
 #define LINUX_KEYCODE_HK_VGA2		    (502)

 #define VIRTUAL_WIN_NUM1				(0x1e)
 #define VIRTUAL_WIN_NUM2				(0x1f)
 #define VIRTUAL_WIN_NUM3				(0x20)
 #define VIRTUAL_WIN_NUM4				(0x21)
 #define VIRTUAL_WIN_NUM5				(0x22)
 #define VIRTUAL_WIN_NUM6				(0x23)
 #define VIRTUAL_WIN_NUM7				(0x24)
 #define VIRTUAL_WIN_NUM8				(0x25)
 #define VIRTUAL_WIN_NUM9				(0x26)
 #define VIRTUAL_WIN_NUM0				(0x27)
 #define VIRTUAL_WIN_ENTER				(0x28)
 #define VIRTUAL_WIN_SPACE				(0x2c)
 #define VIRTUAL_WIN_PAGEUP				(0x4B)
 #define VIRTUAL_WIN_PAGEDN				(0x4e)
 #define VIRTUAL_WIN_RIGHT				(0x4f)
 #define VIRTUAL_WIN_LEFT				(0x50)
 #define VIRTUAL_WIN_DOWN				(0x51)
 #define VIRTUAL_WIN_UP				    (0x52)
 #define VIRTUAL_WIN_BACKSPACE			(0x2a)
 #define VIRTUAL_WIN_MOUSE_RIGHT		(0x65)
 #define VIRTUAL_WIN_F1					(0x3a)
 #define VIRTUAL_WIN_F2					(0x3b)
 #define VIRTUAL_WIN_F3					(0x3c)
 #define VIRTUAL_WIN_F4					(0x3d)
 #define VIRTUAL_WIN_F5					(0x3e)
 #define VIRTUAL_WIN_F6					(0x3f)
 #define VIRTUAL_WIN_F7					(0x40)
 #define VIRTUAL_WIN_F8					(0x41)
 #define VIRTUAL_WIN_F9					(0x42)
 #define VIRTUAL_WIN_F10				(0x43)
 #define VIRTUAL_WIN_F11				(0x44)
 #define VIRTUAL_WIN_F12				(0x45)
 #define VIRTUAL_WIN_TAB				(0x2b)

 #define VIRTUAL_WIN_WIN				(0x08)
 #define VIRTUAL_WIN_ALT				(0x04)

#if 1
 #define IS_VIRTUAL_WIN_NUM0				(0x26)
 #define IS_VIRTUAL_WIN_NUM1				(0x27)
 #define IS_VIRTUAL_WIN_NUM2				(0x28)
 #define IS_VIRTUAL_WIN_NUM3				(0x29)
 #define IS_VIRTUAL_WIN_NUM4				(0x2a)
 #define IS_VIRTUAL_WIN_NUM5				(0x2b)
 #define IS_VIRTUAL_WIN_NUM6				(0x2c)
 #define IS_VIRTUAL_WIN_NUM7				(0x2d)
 #define IS_VIRTUAL_WIN_NUM8				(0x2e)
 #define IS_VIRTUAL_WIN_NUM9				(0x2f)
 #define IS_VIRTUAL_WIN_BACKSPACE			(0x30)
 #define IS_VIRTUAL_WIN_SCREEN				(0x31)
#endif
 #define IS_VIRTUAL_WIN_ENTER				(0x09)
 #define IS_VIRTUAL_WIN_SPACE				(0x08)
 #define IS_VIRTUAL_WIN_PAGEUP				(0x02)
 #define IS_VIRTUAL_WIN_PAGEDN				(0x03)
 #define IS_VIRTUAL_WIN_RIGHT				(0x07)
 #define IS_VIRTUAL_WIN_LEFT				(0x06)
 #define IS_VIRTUAL_WIN_DOWN				(0x05)
 #define IS_VIRTUAL_WIN_UP				    (0x04)
 //#define IS_VIRTUAL_WIN_BACKSPACE			(0x2a)
 #define IS_VIRTUAL_WIN_MOUSE_RIGHT		(0x0a)
 #define IS_VIRTUAL_WIN_F1					(0x0f)
 #define IS_VIRTUAL_WIN_F2					(0x10)
 #define IS_VIRTUAL_WIN_F3					(0x11)
 #define IS_VIRTUAL_WIN_F4					(0x12)
 #define IS_VIRTUAL_WIN_F5					(0x13)
 #define IS_VIRTUAL_WIN_F6					(0x14)
 #define IS_VIRTUAL_WIN_F7					(0x15)
 #define IS_VIRTUAL_WIN_F8					(0x16)
 #define IS_VIRTUAL_WIN_F9					(0x17)
 #define IS_VIRTUAL_WIN_F10				(0x18)
 #define IS_VIRTUAL_WIN_F11				(0x19)
 #define IS_VIRTUAL_WIN_F12				(0x1a)
 //#define IS_VIRTUAL_WIN_TAB				(0x2b)

 #define IS_VIRTUAL_WIN_WIN				(0x0e)
 //#define IS_VIRTUAL_WIN_ALT				(0x04)
 #define IS_VIRTUAL_ALT_F4				(0x0c)
 #define IS_VIRTUAL_ALT_TAB				(0x0d)



#define USBDATAFORBID 		g_forbidUsbData[14]
#define USBCMDSUM			g_forbidUsbData[15]
#define USBCMDCHECKSUM 	g_forbidUsbData[17]

#define UARTDATAFORBID		g_forbidUartData[14]
#define UARTCMDSUM			g_forbidUartData[15]
#define UARTCMDCHECKSUM 	g_forbidUartData[17]

#define CVT_COMBINATION_KEY	g_winVirtualKeyPad[14]
#define CVT_NORMAL_KEY g_winVirtualKeyPad[15]
#define CVT_SUM		   g_winVirtualKeyPad[16]
#define CVT_CHECKSUM   g_winVirtualKeyPad[18]

#define IS_IR_KEY g_IsWinVirtualKey[12]
#define IS_IR_SUM g_IsWinVirtualKey[13]
#define IS_IR_CHECKSUM g_IsWinVirtualKey[15]

#define ISOLUTION_SUM g_startAreaInfo[15]

#define TOGGLE_OPS_SUM g_toggleOPS[8]
#define TOGGLE_OPS_CMD g_toggleOPS[4]

#define TOGGLE_AN_SUM g_toggleAn[8]
#define TOGGLE_AN_CMD g_toggleAn[4]

#define OPS_STAT_SUM	g_getOPSStat[8]

#define ISOLUTION_DATA_CMD  	g_isolutionData[9] 
#define ISOLUTION_DATA_SUM  	g_isolutionData[13]
#define ISOLUTION_CHECK_SUM 	g_isolutionData[15]

class McuGetBack
 {
public:
	static unsigned char g_mcuStat;
	static volatile unsigned char g_OPSStat;
	static pthread_mutex_t g_singletonMutex;
	static McuGetBack* GetInstance(void);
	unsigned char StartBackDataProcThr(void);
	void ExitBackDataProcThr(void);
	void SetBackBuf(unsigned char *pac, int sz);
	void ForbiddenUsbData(unsigned char toggle);
	void ForbiddenUartData(unsigned char toggle);
	void ForbiddenTouch(unsigned char toggle);
	void StartToggleOPS(void);
	void ExitToggleOPS(void);
	void AppStartOPS(void);
	void AppShutDownOPS(void);
	void GetOpsStat(void);
	u8 AppGetOpsStat(void);
	void AppShutDownAndroid(void);
	void GetMcuVersion(char* mcuVer);

	void StartAreaInfo(void);

    static bool g_mUpgradeState;
private:
	static McuGetBack* g_pInstance;
	static unsigned char g_mcuBackPac[BACK_PAC_SIZE];
	static unsigned char g_exitBackProcStat;
	static unsigned char g_lastVgaStat;
	//static std::map<unsigned char ,int> g_Map;
	std::map<unsigned char, int> m_Map;
	std::map<unsigned char, int> m_PadMap;
	//CVTE touchscreen
	std::map<unsigned char, int> m_winMap;
	//isolution touchscreen
	std::map<unsigned char, int> m_IswinMap;
	HANDLE m_backRingBuf;
	pthread_attr_t m_backattr;
	pthread_t m_backpthread;
	timespec m_keypadTime1,m_keypadTime2,m_decKeypadTime;
	unsigned char m_keypadVal;
	static unsigned char g_forbidUsbData[18];
	static unsigned char g_forbidUartData[18];
	static unsigned char g_winVirtualKeyPad[19];
	static unsigned char g_IsWinVirtualKey[16];
	static unsigned char g_toggleOPS[9];
	static unsigned char g_toggleAn[9];
	//static volatile unsigned char g_OPSStat;
	static unsigned char g_getOPSStat[9];

	static unsigned char g_startAreaInfo[16];
	static unsigned char g_isolutionData[16];
	//mcu version
	char mMasterVer, mSlaveVer;

	pthread_t mtoggleThr;
	pthread_t mOpsStatThr;
	pthread_t mlongpressThr;

	unsigned char mLongPressFlag;
	unsigned char mLastpadVal;

	McuGetBack();
	~McuGetBack();
	void backDataProcess();
	static void* mcuGetBackThr(void *arg);
	void vgaDataProc(const unsigned char val);
	void opsStat(const unsigned char cmd, const unsigned char stat);
//	void getOpsStat(void);
	void androidStat(const unsigned char stat);
	void keypadVal(unsigned char val);
	void irVal(const unsigned char val);
	void setUsbDataToggle(unsigned char toggle);
	void setUartDataToggle(unsigned char toggle);
	unsigned char checkSumOfArry(unsigned char* arry, int lenght);
	void winVirtualCheck(unsigned char dat1, unsigned char dat2);
	static void* toggleOPS(void* arg);
	void toggleOPSProc(void);
	void startGetOpsStatProc(void);
	void exitGetOpsStatProc(void);
	static void* getOpsStatProc(void* arg);

	void longpressToDo(void);
	static void* longpressProc(void* arg);
	void exitlongpressThr(void);
	void startlongpressThr(void);
	void isWinVirtualCheck(unsigned char dat1);
 };

 /*0:禁止， 1：开启*/
inline void McuGetBack::setUsbDataToggle(unsigned char toggle)
{
	USBDATAFORBID = toggle;
	USBCMDSUM = checkSumOfArry(&g_forbidUsbData[7], 8);
	USBCMDCHECKSUM = checkSumOfArry(&g_forbidUsbData[0], 17);

	if (0 == toggle)
		ISOLUTION_DATA_CMD = 3;
	else if (1 == toggle)
		ISOLUTION_DATA_CMD = 2;
	else
		DEBUG("please input correct argument.\n");
	ISOLUTION_DATA_SUM = checkSumOfArry(&g_isolutionData[7], 6);
	ISOLUTION_CHECK_SUM = checkSumOfArry(g_isolutionData, 15);
}

inline void McuGetBack::setUartDataToggle(unsigned char toggle)
{
	UARTDATAFORBID = toggle;
	UARTCMDSUM = checkSumOfArry(&g_forbidUartData[7], 8);
	UARTCMDCHECKSUM = checkSumOfArry(&g_forbidUartData[0], 17);

	if (0 == toggle)
		ISOLUTION_DATA_CMD = 6;
	else if (1 == toggle)
		ISOLUTION_DATA_CMD = 5;
	else 
		DEBUG("please input correct argument.\n");
	ISOLUTION_DATA_SUM = checkSumOfArry(&g_isolutionData[7], 6);
	ISOLUTION_CHECK_SUM = checkSumOfArry(g_isolutionData, 15);
}
 inline unsigned char McuGetBack::checkSumOfArry(unsigned char* arry, int lenght)
{
	unsigned char sum =0;
	int i=0;
	for(i=0;i<lenght;i++)
	{
	 	sum+= arry[i];
	}
	return sum;
}

#endif

