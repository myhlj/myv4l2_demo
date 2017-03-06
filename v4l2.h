/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */
#ifndef __V4L2H__
#define __V4L2H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>

#include "v4l2Agent.h"

#define R 0
#define G 1
#define B 2
#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {
	void * start;
	size_t length;
};

class RGB
{
public:
    BYTE r, g, b;
};

class v4l2Agent
{
private:
    /*设备名称*/
    char * m_szdevName;
    /*选取的io方式*/
    io_method m_ioMethod;
    /*v4l2图像结构体*/
    v4l2_format m_fmt;
    /*摄像头文件描述符*/
    int m_nfdCamera;
    /*取得的照片结构体*/
    buffer * m_buffers;
    /*照片索引*/
    unsigned int m_nbuffers;
    /*要返回的照片*/
    char * m_szImage;
    /*要返回的照片的大小*/
    int m_nImageSize;
    /*是否初始化摄像头*/
    bool m_bIsInit;
    /*是否开启了视频流*/
    bool m_bIsStart;

private:
    v4l2Agent();
    ~v4l2Agent();

    static v4l2Agent * m_pSingleInstance;

    int innerInitDevice(void);
    int innerInitRead(unsigned int buffer_size);
    int innerInitMMap(void);
    int innerInitUserPtr(unsigned int buffer_size);
    int innerGetImageLoop(void);
    int innerReadFrame(void);
    int innerProcessImage(const void * p, int size);

    int innerYUYV422ToRGBOrBGR(const BYTE *& szYUYV422Image,
                               BYTE *& szDesImage,
                               const int & nImageWidth,
                               const int & nImageHeigt,
                               const bool & bRGB = true);
    int innerYUYV422ToRGB(const BYTE *& szYUYV422Image,
                          BYTE *& szRGBImage,
                          const int & nYUYVSize);

public:
    static v4l2Agent * GetInstance();
    static void ReleaseInstance();

	// 1
	int nInitCamera(const int & nImageWidth,
					const int & nImageHeigt,
					const char * szDevName,
                    int nDevNameSize,
					const io_method io);

	// 2
    int nStartVedio(void);

	// 3
    int nGetYUYV422ImageSize(void);
    int nGetCurrentYUYV422Image(BYTE *& szYUYV422Image);

	// 4
	int nGetRGBOrBGRImageSize(void);
    int nYUYV422ToRGB(const BYTE *& szYUYV422Image,
					  BYTE *& szRGBImage,
					  const int & nImageWidth,
					  const int & nImageHeigt);

	// 5
    int nYUYV422ToBGR(const BYTE *& szYUYV422Image,
                      BYTE *& szBGRImage,
                      const int & nImageWidth,
                      const int & nImageHeigt);

	// 6
    int nYUYV422ToGray(const BYTE *& szYUYV422Image,
                       BYTE *& szGrayImage,
                       const int & nImageWidth,
                       const int & nImageHeigt);

	// 7
    int nStopVedio(void);

	// 8
    int nUnInitCamera(void);
};

#define V4L2 v4l2Agent::GetInstance()

#endif
