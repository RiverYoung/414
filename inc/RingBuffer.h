/**************************************************************************************
*  Copyright 2003-2015 Hangzhou Hikvision Digital Technology Co., Ltd. 
*  Filename:        RingBuffer.h 
*  Description:     循环缓冲区声明(全工程范围遵守) 
*  Author:          caoyun 
*  Create:          2011-07-26 
*  Modification history:
*      author:    wulonghua
*      date:       2017-01-04
**************************************************************************************/ 
#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include "pthread.h"

#ifndef	OSI_Min
#define	OSI_Min(a, b)	((a) < (b) ? (a):(b))
#endif

#ifndef OSI_NULL
#define OSI_NULL                     (0)
#endif

#ifndef OSI_ERROR
#define OSI_ERROR                  (-1)
#endif

#ifndef OSI_OK
#define OSI_OK                         (0)
#endif

/*循环缓冲区数据结构*/
typedef struct 
{
    unsigned char      *BufBase;     // 环型缓冲区地址
    int                    	    bufLen;       // 环型缓冲区大小

    /*当writeIdx=readIdx,缓冲区为空；writeIdx-readIdx=bufLen,缓冲区为满*/
    int                        writeIdx;     // 环型缓冲区写指针
    int                        readIdx;      // 环型缓冲区读指针
    int                        dataLen;      // 环型缓冲区内容长度

    pthread_mutex_t   mutex;        // 环型缓冲区锁
    int                        bSafe;        //是否线程安全标志
}RING_BUFFER;

typedef void*  HANDLE;

class RingBuffer
{
public:
	static pthread_mutex_t g_singletonMutex;
	static RingBuffer* Get_Instance(void);
	HANDLE  DSI_RingBuf_Init(unsigned int bufSize, int bSafe);
	unsigned int DSI_RingBuf_Write(HANDLE hRingBuf, unsigned char* pSrcBuf, unsigned int len);
	unsigned int DSI_RingBuf_Read(HANDLE hRingBuf, unsigned char* pDstBuf, unsigned int len);
	unsigned int DSI_RingBuf_DateLen(HANDLE hRingBuf);
	unsigned int DSI_RingBuf_FreeLen(HANDLE hRingBuf);
	int DSI_RingBuf_Reset(HANDLE hRingBuf);
	int DSI_RingBuf_Destroy(HANDLE hRingBuf);
private:
	static RingBuffer* g_pInstance;

	RingBuffer();
	~RingBuffer();
	int fls64(unsigned long long x);
	unsigned int fls_long(unsigned long l);
	int fls(int x);
	int OSI_Is_Pow_Of_2(unsigned long long n);
	unsigned int OSI_Roundup_Pow_Of_2(unsigned long long n);
	unsigned int ringBuf_write(RING_BUFFER* pRingBuf, unsigned char* pSrcBuf, unsigned int len);
	unsigned int ringBuf_read(RING_BUFFER* pRingBuf, unsigned char* pDstBuf, unsigned int  len);
};


inline int RingBuffer::OSI_Is_Pow_Of_2(unsigned long long n)
{
    return ( n!=0 && ( (n&(n-1)) == 0) );
}

/**
* fls - find last (most-significant) bit set
* @x: the word to search
*
* This is defined the same way as ffs.
* Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
*/
inline int RingBuffer::fls(int x)
{
    int r = 32;

    if (!x)
        return 0;
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

inline int RingBuffer::fls64(unsigned long long x)
{
    unsigned int h = (x >> 32);
    if (h)
        return fls(h) + 32;
    return fls(x);
}

inline unsigned RingBuffer::fls_long(unsigned long l)
{
    if (sizeof(l) == 4)
        return fls(l);
    return fls64(l);
}

/*****************************************************
* Function:        OSI_Roundup_Pow_Of_2()
* Description:      
* Access Level:    public 
* Input:           OSI_ULONG n
* Output:          
* Return:          
******************************************************/
inline unsigned int RingBuffer::OSI_Roundup_Pow_Of_2(unsigned long long n)
{
    return 1UL << fls_long(n - 1);
}




#endif

