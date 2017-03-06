#ifndef V4L2AGENT_H
#define V4L2AGENT_H

#ifdef __cplusplus
extern "C"{
#endif

#define ERR_NONE                    0           //没有错误
#define ERR_OTHER                  -1           //其他错误
#define ERR_IDENTIFY                1           //识别设备失败
#define ERR_NODEVICE                2           //设备不存在
#define ERR_OPENFAILED              3           //打开设备失败
#define ERR_NOV4L2DEV               4           //不是v4l2设备
#define ERR_NOVIDEOCAPTUREDEV       5           //不是视频设备
#define ERR_NOSUPPORTREADIO         6           //不支持读写IO
#define ERR_NOSUPPORTSTREAMIO       7           //不支持流IO
#define ERR_NOSUPPORTUSERPOINTERIO  8           //不支持用户指针IO
#define ERR_NOSUPPORTMMAP           9           //不支持MMAP
#define ERR_INSUFFICIENTBUFFER      10          //
#define ERR_OUTMEMORY               11          //内存分配失败
#define ERR_NOINIT                  12          //没有初始化
#define ERR_NOSTART                 13          //没有开启视频流
#define ERR_START                   14          //视频流正在开启，不能卸载摄像头，先关闭视频流
#define ERR_MEMORY                  15          //外部没有分配内存

typedef unsigned char BYTE;

// 摄像头读取方式
typedef enum {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
} io_method;

/************************************************************************
 * nInitCamera: 初始化摄像头参数，并设置相应参数
 * Parameters:
 *      nImageWidth[in]: 图像宽度
 *      nImageHeigt[in]: 图像高度
 *      szDevName[in]: 摄像头设备名称，eg. /dev/video0
 *      io[in]: 摄像头读取方式，默认IO_METHOD_MMAP
 * Return Value: int
 *      0 成功
 *    ！0 失败
************************************************************************/
int nInitCamera(const int & nImageWidth,
                const int & nImageHeigt,
                const char * szDevName,
                int nDevNameSize,
                const io_method io = IO_METHOD_MMAP);

/************************************************************************
 * nStartVedio: 开启视频流
 * Parameters:
 * Return Value: int
 *      0 成功
 *    ！0 失败
************************************************************************/
int nStartVedio(void);

/************************************************************************
 * nGetYUYV422ImageSize: 获取YUYV422格式图片大小
 * Parameters:
 * Return Value: int 图片大小
************************************************************************/
int nGetYUYV422ImageSize(void);

/************************************************************************
 * nGetCurrentYUYV422Image: 获取YUYV422图像
 * Parameters:
 *      szYUYV422Image[in, out]: 图像信息，需要调用者申请空间，大小为nGetYUYV422ImageSize函数所获得
 * Return Value: int
 *      0 成功
 *    ！0 失败
************************************************************************/
int nGetCurrentYUYV422Image(BYTE * szYUYV422Image);

/************************************************************************
 * nGetRGBImageSize: 获取RGB格式图片大小
 * Parameters:
 * Return Value: int 图片大小
************************************************************************/
int nGetRGBImageSize(void);

/************************************************************************
 * nYUYV422ToRGB: 将YUYV422图像转换成RGB格式
 * Parameters:
 *      szYUYV422Image[in]: YUYV422图像
 *      szRGBImage[in, out]: RGB图像，需要调用者申请空间，大小为nGetRGBImageSize函数所获得
 *      nImageWidth[in]: 图像宽度
 *      nImageHeight[in]: 图像高度
 * Return Value: int
 *      0 成功
 *    ！0 失败
************************************************************************/
int nYUYV422ToRGB(const BYTE * szYUYV422Image,
                  BYTE * szRGBImage,
                  const int & nImageWidth,
                  const int & nImageHeight);

/************************************************************************
 * nGetBGRImageSize: 获取BGR格式图片大小
 * Parameters:
 * Return Value: int 图片大小
************************************************************************/
int nGetBGRImageSize(void);

/************************************************************************
 * nYUYV422ToBGR: 将YUYV422图像转换成BGR格式
 * Parameters:
 *      szYUYV422Image[in]: YUYV422图像
 *      szBGRImage[in, out]: BGR图像，需要调用者申请空间，大小为nGetBGRImageSize函数所获得
 *      nImageWidth[in]: 图像宽度
 *      nImageHeight[in]: 图像高度
 * Return Value: int
 *      0 成功
 *    ！0 失败
************************************************************************/
int nYUYV422ToBGR(const BYTE * szYUYV422Image,
                  BYTE * szBGRImage,
                  const int & nImageWidth,
                  const int & nImageHeight);

/************************************************************************
 * nYUYV422ToGray: 将YUYV422图像转换成灰度图格式
 * Parameters:
 *      szYUYV422Image[in]: YUYV422图像
 *      szGrayImage[in, out]: 灰度图像，需要调用者申请空间，大小为（图像宽度×图像高度）
 *      nImageWidth[in]: 图像宽度
 *      nImageHeight[in]: 图像高度
 * Return Value: int
 *      0 成功
 *    ！0 失败
************************************************************************/
int nYUYV422ToGray(const BYTE * szYUYV422Image,
                   BYTE * szGrayImage,
                   const int & nImageWidth,
                   const int & nImageHeight);

/************************************************************************
 * nStopVedio: 停止视频流
 * Parameters:
 * Return Value: int
 *      0 成功
 *    ！0 失败
************************************************************************/
int nStopVedio(void);

/************************************************************************
 * nUnInitCamera: 卸载摄像头
 * Parameters:
 * Return Value: int
 *      0 成功
 *    ！0 失败
************************************************************************/
int nUnInitCamera(void);

#ifdef __cplusplus
}
#endif

#endif // V4L2AGENT_H
