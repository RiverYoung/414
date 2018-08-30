/**************************************************************************************
*  Copyright 2003-2015 Hangzhou Hikvision Digital Technology Co., Ltd. 
*  Filename:        ringBuffer.cpp
*  Description:     循环缓冲区实现---参照linux内核循环缓冲区实现(全工程范围遵守) 
*  Author:          caoyun
*  Create:          2011-07-1 
*  Modification history:
*      author:    wulonghua
*      date:       2017-01-04
**************************************************************************************/ 
//#include "libOSI.h"
#include "RingBuffer.h"
#include <stdlib.h>
#include <string.h>
#include <new>
#include <assert.h>
#include "HikLock.h"

RingBuffer* RingBuffer::g_pInstance = NULL;
pthread_mutex_t RingBuffer::g_singletonMutex = PTHREAD_MUTEX_INITIALIZER;

RingBuffer::RingBuffer()
{

}

RingBuffer::~RingBuffer()
{

}

RingBuffer* RingBuffer::Get_Instance(void)
{
	if(NULL == g_pInstance)	
	{	
		/*thread-safe*/
		HikLock HikLock(g_singletonMutex);
		g_pInstance = new (std::nothrow)RingBuffer;
		assert(g_pInstance);
	}
	return g_pInstance;
}


/*************************************************
* Function:         DSI_RingBuf_Init()
* Description:      循环缓冲区初始化函数
* Access Level:     public 
* Input:            bufSize---循环缓冲区大小
* Output:           N/A	
* Return:         	循环缓冲区句柄---成功/OSI_NULL---失败
*************************************************/
HANDLE  RingBuffer::DSI_RingBuf_Init(unsigned int bufSize, int bSafe)
{
    RING_BUFFER *ringBuf = OSI_NULL;
    
    if((ringBuf = (RING_BUFFER*)malloc(sizeof(RING_BUFFER))) == NULL)
    {
        return OSI_NULL;
    }

    /*判断循环缓冲区大小是否为2的次幂，不是的话，就近向上成为2的次幂(很关键，后面的读写算法都依赖于此)！！！*/
    if (!OSI_Is_Pow_Of_2(bufSize)) 
    {
        bufSize = OSI_Roundup_Pow_Of_2(bufSize);
    }
    if((ringBuf->BufBase = (unsigned char*)malloc(bufSize)) == NULL)
    {
        free(ringBuf);
        return OSI_NULL;
    }
    memset(ringBuf->BufBase, 0x0, bufSize);

    ringBuf->bufLen  = bufSize;
    ringBuf->dataLen = 0;
    ringBuf->readIdx = ringBuf->writeIdx = 0;

    //OSI_Mutex_Create(&(ringBuf->mutex));
    //OSI_Mutex_SetName(&(ringBuf->mutex), "ringBuf->mutex");
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, 0);
    pthread_mutex_init(&(ringBuf->mutex), &attr);
    pthread_mutexattr_destroy(&attr);
	

    ringBuf->bSafe = bSafe;

    return (HANDLE)ringBuf;
}

unsigned int RingBuffer::ringBuf_write(RING_BUFFER* pRingBuf, unsigned char* pSrcBuf, unsigned int len)
{
    unsigned int  readIdx, writeIdx, bufLen;
    unsigned int  freeSpace = 0, right = 0;

    assert(pRingBuf != OSI_NULL);
    assert(pSrcBuf != OSI_NULL);

    readIdx = pRingBuf->readIdx;
    writeIdx = pRingBuf->writeIdx;
    bufLen = pRingBuf->bufLen;

    /*循环缓冲区写入策略：有空闲空间就需要写入，而不是必须一次性写入所有数据，充分利用空间。返回实际写入的字节数*/
    freeSpace = OSI_Min(len, bufLen - writeIdx + readIdx);

    /* first put the data starting from writeIdx to buffer end */
    right = OSI_Min(freeSpace, bufLen - (writeIdx & (bufLen - 1)));
    memcpy(pRingBuf->BufBase + (writeIdx & (bufLen - 1)), pSrcBuf, right);

    /* then put the rest (if any) at the beginning of the buffer */  
    memcpy(pRingBuf->BufBase, pSrcBuf+right, freeSpace - right);

    pRingBuf->writeIdx += freeSpace;

    return freeSpace;
}

/*************************************************
* Function:         DS_RingBuf_Write()
* Description:      循环缓冲区写入函数
* Access Level:     public 
* Input:            hRingBuf---循环缓冲区句柄	
*                   pSrpRingBuf---待写入数据源地址
*                   len---待写入数据长度
* Output:           N/A	
* Return:         	实际写入数据的长度---成功/-1---失败
*************************************************/
unsigned int  RingBuffer::DSI_RingBuf_Write(HANDLE hRingBuf, unsigned char* pSrcBuf, unsigned int len)
{
    RING_BUFFER* pRingBuf = OSI_NULL;
    unsigned int  writeLen = 0;

    if(hRingBuf==OSI_NULL || pSrcBuf==OSI_NULL || len==0)
    {
        //OSI_SystemError_Set(OSI_EINVAL);
        return -1;
    }

    pRingBuf = (RING_BUFFER*)hRingBuf;

    /*循环缓冲区写入策略： 有空闲空间就需要写入，而不是必须一次性写入所有数据，充分利用空间。返回实际写入的字节数*/
    if (pRingBuf->bSafe)
    {
        //OSI_Mutex_Lock(&(pRingBuf->mutex));
        pthread_mutex_lock(&(pRingBuf->mutex));

        writeLen = ringBuf_write(pRingBuf, pSrcBuf, len);

        //OSI_Mutex_UnLock(&(pRingBuf->mutex));
        pthread_mutex_unlock(&(pRingBuf->mutex));
    }
    else
    {
        writeLen = ringBuf_write(pRingBuf, pSrcBuf, len);
    }

    return writeLen;
}

unsigned int RingBuffer::ringBuf_read(RING_BUFFER* pRingBuf, unsigned char* pDstBuf, unsigned int  len)
{
    unsigned int  readIdx, writeIdx, bufLen, dataLen;
    unsigned int  readLen = 0, right = 0;

    assert(pRingBuf != OSI_NULL);
    assert(pDstBuf != OSI_NULL);
    
    readIdx = pRingBuf->readIdx;
    writeIdx = pRingBuf->writeIdx;
    bufLen = pRingBuf->bufLen;
    //dataLen = pRingBuf->writeIdx - pRingBuf->readIdx;
	dataLen = writeIdx - readIdx;
    
    /*计算可读数据长度*/
    readLen = OSI_Min(len, dataLen);  

    /* first get the data from readIdx until the end of the buffer */
    right = OSI_Min(readLen, bufLen - (readIdx & (bufLen - 1)));
    memcpy(pDstBuf, pRingBuf->BufBase + (readIdx & (bufLen - 1)), right);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(pDstBuf + right, pRingBuf->BufBase, readLen - right);

    pRingBuf->readIdx += readLen;

    return readLen;
}

/*************************************************
* Function:         DS_RingBuf_Read()
* Description:      循环缓冲区读取数据函数
* Access Level:     public 
* Input:            hRingBuf---循环缓冲区句柄	
*                   pDstBuf---读取数据的目的地地址
*                   len---待读取数据长度
* Output:         	N/A
* Return:         	实际读取数据的长度---成功/-1---失败
*************************************************/
unsigned int RingBuffer::DSI_RingBuf_Read(HANDLE hRingBuf, unsigned char* pDstBuf, unsigned int len)
{
    RING_BUFFER* pRingBuf = OSI_NULL;
    unsigned int  readLen = 0;

    if(hRingBuf==OSI_NULL || pDstBuf==OSI_NULL || len==0)
    {
        //OSI_SystemError_Set(OSI_EINVAL);
        return -1;
    }

    pRingBuf = (RING_BUFFER*)hRingBuf;

    /*循环缓冲区读取策略：缓冲区有<=len数据就读取多少数据。返回实际读取的字节数*/
    if (pRingBuf->bSafe)
    {
        //OSI_Mutex_Lock(&(pRingBuf->mutex));
        pthread_mutex_lock(&(pRingBuf->mutex));

        readLen = ringBuf_read(pRingBuf, pDstBuf, len);

        //OSI_Mutex_UnLock(&(pRingBuf->mutex));
        pthread_mutex_unlock(&(pRingBuf->mutex));
    } 
    else
    {
        readLen = ringBuf_read(pRingBuf, pDstBuf, len);
    }

    return readLen;
}

/*************************************************
* Function:         DS_RingBuf_DataLen()
* Description:      循环缓冲区有效数据长度
* Access Level:     public 
* Input:            hRingBuf---循环缓冲区句柄
* Output:           N/A	
* Return:         	循环缓冲区有效数据长度---成功/OSI_ERROR---失败
*************************************************/
unsigned int RingBuffer::DSI_RingBuf_DateLen(HANDLE hRingBuf)
{
    RING_BUFFER* pRingBuf = OSI_NULL;
    unsigned int  dataLen = 0;

    if(hRingBuf == OSI_NULL)
    {
        //OSI_SystemError_Set(OSI_EINVAL);
        return OSI_ERROR;
    }

    pRingBuf = (RING_BUFFER*)hRingBuf;

    /*该策略使得writeIdx一定大于readIdx, 但是writeIdx-readIdx<=bufLen*/
    if (pRingBuf->bSafe)
    {
        //OSI_Mutex_Lock(&(pRingBuf->mutex));
        pthread_mutex_lock(&(pRingBuf->mutex));

        dataLen = pRingBuf->writeIdx - pRingBuf->readIdx;
        pRingBuf->dataLen = dataLen;

        //OSI_Mutex_UnLock(&pRingBuf->mutex);
        pthread_mutex_unlock(&(pRingBuf->mutex));
    } 
    else
    {
        dataLen = pRingBuf->writeIdx - pRingBuf->readIdx;
        pRingBuf->dataLen = dataLen;
    }

    return dataLen;
}

/*************************************************
* Function:         DS_RingBuf_FreeLen()
* Description:      循环缓冲区空闲空间长度
* Access Level:     public 
* Input:            hRingBuf---循环缓冲区句柄
* Output:           N/A	
* Return:         	循环缓冲区空闲长度---成功/OSI_ERROR---失败
*************************************************/
unsigned int RingBuffer::DSI_RingBuf_FreeLen(HANDLE hRingBuf)
{
    RING_BUFFER* pRingBuf = OSI_NULL;
    unsigned int  dataLen = 0, freeLen = 0;

    if(hRingBuf == OSI_NULL)
    {
        //OSI_SystemError_Set(OSI_EINVAL);
        return OSI_ERROR;
    }

    pRingBuf = (RING_BUFFER*)hRingBuf;

    /*该策略使得writeIdx一定大于readIdx, 但是writeIdx-readIdx<=bufLen*/
    if (pRingBuf->bSafe)
    {
        //OSI_Mutex_Lock(&(pRingBuf->mutex));
        pthread_mutex_lock(&(pRingBuf->mutex));

        dataLen = pRingBuf->writeIdx - pRingBuf->readIdx;
		freeLen = pRingBuf->bufLen - dataLen;

        //OSI_Mutex_UnLock(&pRingBuf->mutex);
        pthread_mutex_unlock(&(pRingBuf->mutex));
    } 
    else
    {
        dataLen = pRingBuf->writeIdx - pRingBuf->readIdx;
		freeLen = pRingBuf->bufLen - dataLen;
    }

    return freeLen;
}

/*****************************************************
* Function:        DSI_RingBuf_Reset()
* Description:     重置循环缓冲区 
* Access Level:    public 
* Input:           hRingBuf
* Output:          N/A 
* Return:          OSI_OK---成功/OSI_ERROR---失败  
******************************************************/
int RingBuffer::DSI_RingBuf_Reset(HANDLE hRingBuf)
{
    RING_BUFFER* pRingBuf = OSI_NULL;
    //OSI_UINT32  dataLen = 0;

    if(hRingBuf == OSI_NULL)
    {
        //OSI_SystemError_Set(OSI_EINVAL);
        return OSI_ERROR;
    }

    pRingBuf = (RING_BUFFER*)hRingBuf;

    if (pRingBuf->bSafe)
    {
        //OSI_Mutex_Lock(&(pRingBuf->mutex));
        pthread_mutex_lock(&(pRingBuf->mutex));

        pRingBuf->readIdx = pRingBuf->writeIdx = 0;

        //OSI_Mutex_UnLock(&pRingBuf->mutex);
        pthread_mutex_unlock(&(pRingBuf->mutex));
    } 
    else
    {
        pRingBuf->readIdx = pRingBuf->writeIdx = 0;
    }

    return OSI_OK;
}

/*************************************************
* Function:         DS_RingBuf_Destroy()
* Description:      循环缓冲区销毁函数 
* Access Level:     public 
* Input:            hRingBuf---循环缓冲区句柄	
* Output:         	N/A
* Return:         	OSI_OK/OSI_ERROR
*************************************************/
int  RingBuffer::DSI_RingBuf_Destroy(HANDLE hRingBuf)
{
    RING_BUFFER* pRingBuf = OSI_NULL;

    if(hRingBuf == OSI_NULL)
    {
        //OSI_SystemError_Set(OSI_EINVAL);
        return OSI_ERROR;
    }

    pRingBuf = (RING_BUFFER*)hRingBuf;

    //OSI_Mutex_Lock(&(pRingBuf->mutex));
    pthread_mutex_lock(&(pRingBuf->mutex));
    free(pRingBuf->BufBase);
    pRingBuf->BufBase = OSI_NULL;
    //OSI_Mutex_UnLock(&(pRingBuf->mutex));
    pthread_mutex_unlock(&(pRingBuf->mutex));

    //OSI_Mutex_Destroy(&(pRingBuf->mutex));
    pthread_mutex_destroy(&(pRingBuf->mutex));
    free(pRingBuf);
    pRingBuf = OSI_NULL;

    return OSI_OK;
}


