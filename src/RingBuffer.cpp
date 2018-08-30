/**************************************************************************************
*  Copyright 2003-2015 Hangzhou Hikvision Digital Technology Co., Ltd. 
*  Filename:        ringBuffer.cpp
*  Description:     ѭ��������ʵ��---����linux�ں�ѭ��������ʵ��(ȫ���̷�Χ����) 
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
* Description:      ѭ����������ʼ������
* Access Level:     public 
* Input:            bufSize---ѭ����������С
* Output:           N/A	
* Return:         	ѭ�����������---�ɹ�/OSI_NULL---ʧ��
*************************************************/
HANDLE  RingBuffer::DSI_RingBuf_Init(unsigned int bufSize, int bSafe)
{
    RING_BUFFER *ringBuf = OSI_NULL;
    
    if((ringBuf = (RING_BUFFER*)malloc(sizeof(RING_BUFFER))) == NULL)
    {
        return OSI_NULL;
    }

    /*�ж�ѭ����������С�Ƿ�Ϊ2�Ĵ��ݣ����ǵĻ����ͽ����ϳ�Ϊ2�Ĵ���(�ܹؼ�������Ķ�д�㷨�������ڴ�)������*/
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

    /*ѭ��������д����ԣ��п��пռ����Ҫд�룬�����Ǳ���һ����д���������ݣ�������ÿռ䡣����ʵ��д����ֽ���*/
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
* Description:      ѭ��������д�뺯��
* Access Level:     public 
* Input:            hRingBuf---ѭ�����������	
*                   pSrpRingBuf---��д������Դ��ַ
*                   len---��д�����ݳ���
* Output:           N/A	
* Return:         	ʵ��д�����ݵĳ���---�ɹ�/-1---ʧ��
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

    /*ѭ��������д����ԣ� �п��пռ����Ҫд�룬�����Ǳ���һ����д���������ݣ�������ÿռ䡣����ʵ��д����ֽ���*/
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
    
    /*����ɶ����ݳ���*/
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
* Description:      ѭ����������ȡ���ݺ���
* Access Level:     public 
* Input:            hRingBuf---ѭ�����������	
*                   pDstBuf---��ȡ���ݵ�Ŀ�ĵص�ַ
*                   len---����ȡ���ݳ���
* Output:         	N/A
* Return:         	ʵ�ʶ�ȡ���ݵĳ���---�ɹ�/-1---ʧ��
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

    /*ѭ����������ȡ���ԣ���������<=len���ݾͶ�ȡ�������ݡ�����ʵ�ʶ�ȡ���ֽ���*/
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
* Description:      ѭ����������Ч���ݳ���
* Access Level:     public 
* Input:            hRingBuf---ѭ�����������
* Output:           N/A	
* Return:         	ѭ����������Ч���ݳ���---�ɹ�/OSI_ERROR---ʧ��
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

    /*�ò���ʹ��writeIdxһ������readIdx, ����writeIdx-readIdx<=bufLen*/
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
* Description:      ѭ�����������пռ䳤��
* Access Level:     public 
* Input:            hRingBuf---ѭ�����������
* Output:           N/A	
* Return:         	ѭ�����������г���---�ɹ�/OSI_ERROR---ʧ��
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

    /*�ò���ʹ��writeIdxһ������readIdx, ����writeIdx-readIdx<=bufLen*/
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
* Description:     ����ѭ�������� 
* Access Level:    public 
* Input:           hRingBuf
* Output:          N/A 
* Return:          OSI_OK---�ɹ�/OSI_ERROR---ʧ��  
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
* Description:      ѭ�����������ٺ��� 
* Access Level:     public 
* Input:            hRingBuf---ѭ�����������	
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


