/*****************************************************
 Copyright 2003-2017 Hangzhou Hikvision Digital Technology Co., Ltd.
 FileName: TsDev.cpp
 Description: TODO
 Others:
 Author: wulonghua
 Date: 2016-02-16
 Version: V1.0.0
 Modification History:
 *****************************************************/

#include "TsDev.h"
#include <new>
#include <assert.h>
#include <stdio.h>
#include "debug.h"
#include "HikLock.h"
#include "mapi_utility.h"
#include "VirtualInput.h"
#include "customized_display_onoff.h"


#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/stat.h>
#include <stdint.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>


char TsDev::g_flag = 0;
TsDev* TsDev::g_pInstance = NULL;
pthread_mutex_t TsDev::g_singletonMutex = PTHREAD_MUTEX_INITIALIZER;
char TsDev::g_ExitTsDevUartStat = 1;
libusb_device_handle* TsDev::g_devHandle = NULL;
unsigned char TsDev::g_recvDat[TOUCHDEVPACSIZE] = {0};
char TsDev::g_pressflag = 0;
//struct usb_g_devHandle * TsDev::touchg_devHandle = NULL;
int TsDev::g_ecoflag = 0;
unsigned char TsDev::g_areaflag = 1;
unsigned char TsDev::g_osdCnt = 0;
char TsDev::g_eraserFlag = 0;

TsDev::TsDev()
{
	g_ExitTsDevUartStat = 1;
	m_ctx = NULL;
	g_devHandle = NULL;

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
	memset(m_coordinate, 0, sizeof(m_coordinate));
	memset(m_eraser, 0, sizeof(m_eraser));
	memset(&m_TsDevThread, 0, sizeof(m_TsDevThread));
	
}

TsDev::~TsDev()
{
	delete g_pInstance;
	g_pInstance = NULL;
}

TsDev* TsDev::GetInstance(void)
{
	if(NULL == g_pInstance)
	{
		/*singleton thread-safe*/
		HikLock m_hikLock(g_singletonMutex);
		g_pInstance = new (std::nothrow)TsDev;
		assert(g_pInstance);
	}

	return g_pInstance;
}

void* TsDev::touchDevThr(void* arg)
{
	assert(arg);
	//((TsDev*)arg)->openTsDev();
	((TsDev*)arg)->gestrueProc();

	return NULL;
}


unsigned char TsDev::StartTsDevThrProcess(void)
{
	unsigned char ret;

	pthread_attr_init(&m_TsDevAttr);
	pthread_attr_setstacksize(&m_TsDevAttr, PTHREAD_STACK_SIZE);
	ret = PTH_RET_CHK(pthread_create(&m_TsDevThread,  &m_TsDevAttr, touchDevThr, this));

	/*0:create mcu return back thread success;1: failed*/
	return (ret != 0) ? 0 : 1;
}

void TsDev::ExitTsDevThrProcess(void)
{
	if(0 == g_ExitTsDevUartStat)
	{
		g_ExitTsDevUartStat = 1;
		void* thread_result;
		unsigned char ret;
		if(m_TsDevThread != 0)
		{
			ret = PTH_RET_CHK(pthread_join(m_TsDevThread, &thread_result));
			if(ret != 0)
				DEBUG("TsDev thread join failed\n");
			else
			{
				m_TsDevThread = 0;
				DEBUG("hanging TsDev thread success\n");
			}
		}
		else
			DEBUG("TsDev thread has hanged\n");

	}
	else
		DEBUG("TsDev thread is not running\n");
}

void TsDev::printDevs(libusb_device** devs)
{
	libusb_device* dev;
	int i = 0;

	while ((dev = devs[i++]) != NULL)
	{
		struct libusb_device_descriptor desc;
		int err = libusb_get_device_descriptor(dev, &desc);
		if (err < 0)
		{
			DEBUG("failed to get device descriptor.\n");
			return;
		}

		DEBUG("%04x:%04x (bus %d, device %d).\n", desc.idVendor, desc.idProduct,\
			libusb_get_bus_number(dev), libusb_get_device_address(dev));

		if (desc.idVendor == 0x483)
			g_flag ++;
	}
}

int TsDev::openTsDev(void)
{
	libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices

	int err;
	ssize_t cnt;

	err = libusb_init(&m_ctx);
	if (err <0)
	{
		DEBUG("init error.\n");
		return err;
	}

    while (g_devHandle == NULL)
    {
    	/**
		err = libusb_init(&m_ctx);
		if (err <0)
		{
			DEBUG("init error.\n");
			return err;
		}
		**/

		libusb_set_debug(m_ctx, LIBUSB_LOG_LEVEL_INFO);
		cnt = libusb_get_device_list(m_ctx, &devs);
		if (cnt < 0)
		{
			DEBUG("get devices lis error.\n");
			return cnt;
		}
		g_devHandle = libusb_open_device_with_vid_pid(m_ctx, TOUCHDEV_VENDOR_ID, TOUCHDEV_PRODUCT_ID);

		printDevs(devs);

		libusb_free_device_list(devs, 1);
		usleep(1000*1000);
	}

	//printDevs(devs);

	DEBUG("TsDev open success.\n");

	if (1 == libusb_kernel_driver_active(g_devHandle, 0))//find out if kernel driver is attached
	{
		DEBUG("TsDev kernel driver active.\n");
		if (0 ==libusb_detach_kernel_driver(g_devHandle, 0))//detach it
			DEBUG("TsDev kernel driver detached.\n");
	}

	err = libusb_claim_interface(g_devHandle, 0);//claim interface 0 (the first) of device
	if (err < 0)
	{
		DEBUG("cannot claim interface.\n");
		return err;
	}

	DEBUG("claim interface success.\n");

	return 0;
}

/*Synchronous device I/O*/
//add for save data to usb
FILE *g_fd = NULL;
FILE *g_fd_recvDataCnt = NULL;

void findUSBPath(char *filename,char fd_choose)
{	
	DIR *dirptr = NULL;	
	struct dirent *entry = NULL;	
	char filePath[80];	
	memset(filePath, 0, 80);

	if((dirptr = opendir("/mnt/usb"))== NULL)
	{         		
		DEBUG("/mnt/usb opendir failed!");    
	}    
	else	
	{	
		while((entry = readdir(dirptr)) != NULL)	
		{		
			if(entry->d_name[0] == '.') 
			{			
				  continue;	
			} 		
			strcat(filePath,"/mnt/usb/");	
			DEBUG("filePath = %s",filePath); 	
			strcat(filePath,entry->d_name);	
			DEBUG("filePath = %s",filePath); //	
			//sprintf(fileName,"/mnt/usb/%s",entry->d_name);//	
			break;		
		}	
	}
	strcat(filePath,"/");
	strcat(filePath,filename);
	DEBUG("filePath = %s",filePath);

	if(1 == fd_choose)
	{
		g_fd = fopen(filePath, "wb+");
		if(NULL == g_fd)  
		{       
			perror("Open fdFileName failed\n");    
		}
	}
	else if(2 == fd_choose)
	{
		g_fd_recvDataCnt = fopen(filePath, "wb");	//二进制只写模式打开，覆盖之前的数据
		if(NULL == g_fd_recvDataCnt)  
		{       
			perror("Open fdFileName failed\n");    
		// exit(-1);   
		}
	}
}

//void findUSBPath(void)
//{	
//	DIR *dirptr = NULL;	
//	struct dirent *entry = NULL;	
//	char filePath[80];	
//	char recvDataCntFilePath[80];
//	memset(filePath, 0, 80);
//	memset(recvDataCntFilePath, 0, 80);
//	
//	if((dirptr = opendir("/mnt/usb"))== NULL)
//	{         		
//		DEBUG("/mnt/usb opendir failed!");    
//	}    
//	else	
//	{	
//		while((entry = readdir(dirptr)) != NULL)	
//		{		
//			if(entry->d_name[0] == '.') 
//			{			
//				  continue;	
//			} 		
//			strcat(filePath,"/mnt/usb/");	
//			DEBUG("filePath = %s",filePath); 	
//			strcat(filePath,entry->d_name);	
//			DEBUG("filePath = %s",filePath); //	
//			//sprintf(fileName,"/mnt/usb/%s",entry->d_name);//	
//			break;		
//		}	
//	}
//	strcat(recvDataCntFilePath,filePath);
//	strcat(recvDataCntFilePath,"/IRDataToUSBCnt.txt");
//	strcat(filePath,"/IRDataToUSB.txt");
//	//strcat(fileName,"/IRDataToUSB.txt");
//	DEBUG("recvDataCntFilePath = %s",recvDataCntFilePath);
//	DEBUG("filePath = %s",filePath);
//	//DEBUG("fileName = %s\n",fileName); 	
//	//g_fd = fopen(filePath,O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
//	g_fd = fopen(filePath, "wb+");
//	g_fd_recvDataCnt = fopen(recvDataCntFilePath,"wb+");
//	if(NULL == g_fd)  
//	{       
//		perror("Open fdFileName failed\n");    
//		// exit(-1);   
//	}
//}

bool copyFileToUSB(unsigned char *fileData)
	{	
		int length = 62;  //???Ϊʲô��62
		//unsigned char *ptr =  NULL;
		//int cntWrite = 0;
		bool flag = true;
		DEBUG("fileData = %s\n",fileData);
		DEBUG("length = %d\n",length);

		if(length < 0)	
			{		
			    printf("Get File Data Failed\n");		
				flag = false;		
				return flag;
			}		
		
		//ptr = fileData;	
		//fwrite(FAR const void * ptr, size_t size, size_t n_items, FAR FILE * stream)	
		/*
		while((cntWrite = fwrite(ptr,1,length,g_fd)) != 0)
			{				
				if(cntWrite < 0)	
					{			
						perror("Write error!\n");	
						flag = false;		
						break;	
						}		
				else if(cntWrite == length)		
					{		
						break;	
					}	
				else if(cntWrite > 0)
					{		
						ptr += cntWrite;
						length -= cntWrite;		
					}	
				}	
			*/ 
		for(int i =0; i < 62; i++)
		{
			fprintf(g_fd,"%02x",fileData[i]);
		}
		return flag;
	}


void TsDev::gestrueProc(void)
{
	int actual;
	int err;
	openTsDev();
	int temp = 0;
	int g_recvDat_cnt = 0;
	
	char IRDataToUSB[64] = "IRDataToUSB.txt";
	char cntIRDataToUSB[64] = "CntIRDataToUSB.txt";
	
	while (1)
	{
		err = libusb_bulk_transfer(g_devHandle, TOUCHDEV_INEP, g_recvDat, TOUCHDEVPACSIZE, &actual, 100);
		//DEBUG("err = %d, actual = %d.\n", err, actual);
		if ((TOUCHDEVPACSIZE == actual)&&(0 == err))
		{	
		    if(NULL == g_fd)
		    {
		        findUSBPath(IRDataToUSB,1);
				if(NULL != g_fd)
				{
				   copyFileToUSB(g_recvDat);
				}
		    }
			else
			{
				copyFileToUSB(g_recvDat);
			}

			g_recvDat_cnt++;
			if(NULL == g_fd_recvDataCnt)
			{
				findUSBPath(cntIRDataToUSB, 2);
				if(NULL != g_fd_recvDataCnt)
				{
					fprintf(g_fd_recvDataCnt, "%04d ", g_recvDat_cnt);
					//fwrite(&g_recvDat_cnt, sizeof(int), 1, g_fd_recvDataCnt);
					//fprintf(g_fd_recvDataCnt,"%02x",g_recvDat_cnt);
				}
			}
			else
			{
				fprintf(g_fd_recvDataCnt, "%04d ", g_recvDat_cnt);
				//fwrite(&g_recvDat_cnt, sizeof(int), 1, g_fd_recvDataCnt);
				//fprintf(g_fd_recvDataCnt,"%02x",g_recvDat_cnt);
			}
			DEBUG("g_recvDat_cnt = %d\n",g_recvDat_cnt);
			DEBUG("g_fd_recvDataCnt = %x\n\n",(unsigned int )g_fd_recvDataCnt);
			
			//DEBUG("recv %d bytes, byte[0] = %d, byte[1] = %d, byte[2] = %d, byte[3] = %d.\n",actual, g_recvDat[0], g_recvDat[1], g_recvDat[2], g_recvDat[3]);
			if (temp == 0)
			{
				if (0x00== g_recvDat[2])
					g_areaflag = CVT_TOUCH;
				else if (0x01 == g_recvDat[2])
					g_areaflag = ISOLUTION_TOUCH;
				//DEBUG("g_areaflag = %d, id = %d.\n", g_areaflag, g_recvDat[2]);
				temp = 1;
			}
			gestrueRecognition();
			//memset(g_recvDat, 0, sizeof(g_recvDat));
			//set backlight on
			if (1 == g_ecoflag)
			{
				//enable backlight
				Plugins_OnOff(1);
				g_ecoflag = 0;
			}
		}
		else
		{
			//memset(m_coordinate, 0, sizeof(m_coordinate));
			memset(m_eraser, 0, sizeof(m_eraser));
			g_eraserFlag = 0;
			g_pressflag = 0;
			//DEBUG("g_eraserFlag = %d.\n", g_eraserFlag);
		}
	}
}


int TsDev::GetEcoFlag(void)
{
	return g_ecoflag;
}

void TsDev::SetEcoFlag(int val)
{
	//lock
	//autolock()
	g_ecoflag = val;
}

void TsDev::gestrueRecognition(void)
{
	if (REPORTID == g_recvDat[0])
	{
		//every times
		unsigned long uart_Xpoint1 = g_recvDat[3]+g_recvDat[4]*0xFF;
		unsigned long uart_Ypoint1 = g_recvDat[5]+g_recvDat[6]*0xFF;
		unsigned long uart_Xpoint2 = g_recvDat[13]+g_recvDat[14]*0xFF;
		unsigned long uart_Ypoint2 = g_recvDat[15]+g_recvDat[16]*0xFF;

		#if ERASER_FLAG
		unsigned long uart_width1 = g_recvDat[7]+g_recvDat[8]*0xFF;;
		unsigned long uart_height1 = g_recvDat[9]+g_recvDat[10]*0xFF;
		//DEBUG("width1 = %ld, uart_height1 = %ld.\n", uart_width1, uart_height1);
		//DEBUG("width1 = %ld, .\n", uart_width1);
		#endif
		//DEBUG("touch state = %d, uart_Xpoint1 =%ld, uart_width1 = %ld.\n", g_recvDat[1], uart_Xpoint1, uart_width1);
		if (g_recvDat[1] != 0x0)
		{
			if (g_pressflag ==  0)
			{
				if ((uart_Ypoint1 >32500)&&(uart_Ypoint1 <= 32767)
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
				else if ((g_recvDat[61] == 0x02)&&(g_recvDat[11] != 0x0))
				{
					m_atTmpX1 = uart_Xpoint1;
					m_atTmpY1 = uart_Ypoint1;
					m_atTmpX2 = uart_Xpoint2;
					m_atTmpY2 = uart_Ypoint2;
					m_atStartTime = mapi_time_utility::GetTime0();
					g_pressflag = 2;			//ASSISTTOUCH GESTRUE
					//DEBUG("######b_pressflag = %d, start_time = %ld\n",b_pressflag,start_time);
				}
				#if ERASER_FLAG
				//else if (((CVT_CONVERSION(uart_width1) >= 1639)/** ||(uart_height1 >= 500)**/)&&(uart_width1 <= 2000)&&(uart_height1 <= 2000)&&(g_areaflag = CVT_TOUCH))
				else if ((g_areaflag == CVT_TOUCH)&&((CVT_X_CONVERSION(uart_width1) >= 1200)||(CVT_X_CONVERSION(uart_height1) >= 2100)))
				{
					g_pressflag = 3;
				}
				else if ((g_areaflag == ISOLUTION_TOUCH)&&((uart_width1 >= 1200)||(uart_height1 >= 2100)))
				{
					g_pressflag = 3;
				}
				#endif
				else if ((uart_Xpoint1 > 32500) && (uart_Xpoint1 <= 32767))
				{
					m_firstX= uart_Xpoint1;
					m_firstY = uart_Ypoint1;
					m_osdTmpX= uart_Xpoint1;
					m_osdTmpY= uart_Ypoint1;
					//m_osdStartTime = mapi_time_utility::GetTime0();
					g_pressflag = 4;                 //new OSD GESTRUE
				}
				else if ((uart_Xpoint1 > 0) && (uart_Xpoint1 <= 267))
				{
					m_firstX= uart_Xpoint1;
					m_firstY = uart_Ypoint1;
					m_osdTmpX= uart_Xpoint1;
					m_osdTmpY= uart_Ypoint1;
					//m_osdStartTime = mapi_time_utility::GetTime0();
					g_pressflag = 5;                 //new OSD GESTRUE
				}
			}
			else
			{
				if(g_pressflag == 1)
				{
					//keep pressing bottom
					#if 0
					if((abs(m_osdTmpX- uart_Xpoint1)<300)&&(abs(m_osdTmpY- uart_Ypoint1)<300))
					{
						DEBUG("#### OSD_det_time = %ld\n", mapi_time_utility::GetTime0()-m_osdStartTime);
						if(/**(broad_flag == 0)&&**/(mapi_time_utility::GetTime0()-m_osdStartTime>= 1000))
						{
							VirtualInput::Get_Instance()->Send_Event(139);
							g_pressflag = 0;
						}
					}
					#endif
					#if 1//bottom->up
					//DEBUG("delt1 = %ld, delt2 = %ld.\n", m_firstX - uart_Xpoint1, m_osdTmpY - uart_Ypoint1);
					if ((abs(m_firstX - uart_Xpoint1)<500) && (m_osdTmpY - uart_Ypoint1 < 500))
					{
						g_osdCnt ++;
						//DEBUG("g_osdCnt = %d.\n", g_osdCnt);
						m_osdTmpX = uart_Xpoint1;
						m_osdTmpY = uart_Ypoint1;
						if ((abs(m_firstY - uart_Ypoint1)>2000) && (g_osdCnt >= 5))
						{
							//VirtualInput::GetInstance()->SendEvent(139);
							g_pressflag = 0;
							g_osdCnt = 0;

						}

					}
					#endif
					else
					{
						g_pressflag  = 0;
						g_osdCnt = 0;
					}
				}
				else if(g_pressflag == 2)
				{
					if((abs(m_atTmpX1 -uart_Xpoint1)<200)&&(abs(m_atTmpY1 -uart_Ypoint1)<200)&&(abs(m_atTmpX2 -uart_Xpoint2)<200)&&(abs(m_atTmpY2 -uart_Ypoint2)<200)
						&&(abs(uart_Xpoint1-uart_Xpoint2)<2000)&&(abs(uart_Ypoint1-uart_Ypoint2)<2000)&&(g_recvDat[61] == 0x02))
					{
						//DEBUG("#### AST_det_time = %ld\n", mapi_time_utility::GetTime0() -m_atStartTime);
						if(/**(broad_flag == 0)&&**/(mapi_time_utility::GetTime0() -m_atStartTime)>=800)
						{
							//SendEventToAndroid(168);
							VirtualInput::GetInstance()->SendEvent(168);
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
				#if ERASER_FLAG
				else if ((g_pressflag == 3) && (g_eraserFlag == 0))
				{
					g_eraserFlag = 1;
					m_eraser[0] = uart_Xpoint1;
					m_eraser[1] = uart_Ypoint1;
					//m_eraser[2] = uart_width1 >= uart_height1 ? uart_width1 : uart_height1;
					if (g_areaflag == ISOLUTION_TOUCH)
						m_eraser[2] = uart_width1 / 4;
					else if (g_areaflag == CVT_TOUCH)
						m_eraser[2] = uart_width1 >= uart_height1 ? uart_width1 : uart_height1;
					VirtualInput::GetInstance()->SendEvent(159);
					DEBUG("send eraser event.\n");
					g_pressflag = 0;
				}
				#endif
				else if (g_pressflag == 4)
				{
					#if 1//right->left
					//DEBUG("delt1 = %ld, delt2 = %ld.\n", m_firstX - uart_Xpoint1, m_osdTmpY - uart_Ypoint1);
					if ((abs(m_firstY - uart_Ypoint1)<800) && (m_osdTmpX - uart_Xpoint1 < 800))
					{
						m_osdTmpX = uart_Xpoint1;
						m_osdTmpY = uart_Ypoint1;
						if ((abs(m_firstX - uart_Xpoint1)>2000))
						{
							VirtualInput::GetInstance()->SendEvent(139);
							g_pressflag = 0;
							g_osdCnt = 0;

						}

					}
					else
					{
						g_pressflag  = 0;
					}
					#endif
				}
				else if (g_pressflag == 5)
				{
					#if 1//right->left
					//DEBUG("delt1 = %ld, delt2 = %ld.\n", m_firstX - uart_Xpoint1, m_osdTmpY - uart_Ypoint1);
					if ((abs(m_firstY - uart_Ypoint1)<800) && (abs(m_osdTmpX - uart_Xpoint1) < 800))
					{
						m_osdTmpX = uart_Xpoint1;
						m_osdTmpY = uart_Ypoint1;
						if ((abs(m_firstX - uart_Xpoint1)>2000))
						{
							VirtualInput::GetInstance()->SendEvent(507);
							g_pressflag = 0;
							g_osdCnt = 0;

						}

					}
					else
					{
						g_pressflag  = 0;
					}
					#endif
				}
				else
				{
					g_pressflag = 0;
				}

			}

		}
	}
}

/**
void TsDev::instantiate()
{
	TsDev::GetInstance();
	if (g_pInstance != NULL)
		defaultServiceManager()->addService(String16(TOUCH_SERVICE),  g_pInstance);
}
**/
int TsDev::GetCount(void)
{
	return g_recvDat[61];
}

float TsDev::GetX(int num)
{
	DEBUG("get x coordinate.\n");
	if (num <= 6)
		return X_CONVERT((float)(g_recvDat[3+(10*(num - 1))] + g_recvDat[4+(10*(num - 1))] * 256));
	else
		return 0;
}

float TsDev::GetY(int num)
{
	if (num <= 6)
		return  Y_CONVERT((float)(g_recvDat[5+(10*(num - 1))] + g_recvDat[6+(10*(num - 1))] * 256));
	else
		return 0;
}

float TsDev::GetWidth(int num)
{
    if (g_areaflag == CVT_TOUCH)
    {
	    if (num <= 6)
		    return  CVT_X_CONVERSION(g_recvDat[7+(10*(num - 1))] + g_recvDat[8+(10*(num - 1))] * 0xff);
	    else
		    return 0;
    }
    else if(g_areaflag == ISOLUTION_TOUCH)
    {
	    if (num <= 6)
		    return  g_recvDat[7+(10*(num - 1))] + g_recvDat[8+(10*(num - 1))] * 0xff;
	    else
		    return 0;
    }
    else
        return 0;

}

float TsDev::GetHeight(int num)
{
	if (num <= 6)
		return  g_recvDat[9+(10*(num - 1))] + g_recvDat[10+(10*(num - 1))] * 0xff;
	else
		return 0;
}

unsigned long* TsDev::GetCoordinate(void)
{
	return m_coordinate;
}

unsigned long* TsDev::GetEraser(void)
{
	return m_eraser;
}

void TsDev::closeTsDev(void)
{
	libusb_release_interface(g_devHandle, 0);
    	libusb_close(g_devHandle); //close the device we opened
    	libusb_exit(m_ctx); //needs to be called to end the
}
