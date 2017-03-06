/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */
/***********************************************************************************************/
#include "v4l2.h"
/***********************************************************************************************/
void errno_exit(const char * s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
}
/***********************************************************************************************/
int xioctl(int fd, int request, void * arg)
{
	int nRet;

	do {
		nRet = ioctl(fd, request, arg);
	} while (-1 == nRet && EINTR == errno);

	return nRet;
}
/***********************************************************************************************/
static int convert_yuv_to_rgb_pixel(int y, int u, int v)
{
    unsigned int pixel32 = 0;
    BYTE *pixel = (BYTE *)&pixel32;
    int r, g, b;
    r = y + (1.370705 * (v-128));
    g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
    b = y + (1.732446 * (u-128));
    if(r > 255) r = 255;
    if(g > 255) g = 255;
    if(b > 255) b = 255;
    if(r < 0) r = 0;
    if(g < 0) g = 0;
    if(b < 0) b = 0;
    pixel[0] = r * 220 / 256;
    pixel[1] = g * 220 / 256;
    pixel[2] = b * 220 / 256;
    return pixel32;
}
/***********************************************************************************************/
RGB yuvTorgbOrbgr(BYTE Y, BYTE U, BYTE V, bool bRGB = true)
{
    RGB rgb;
    if (bRGB)
    {
        rgb.r = (int)((Y&0xff) + 1.4075 * ((V&0xff)-128));
        rgb.g = (int)((Y&0xff) - 0.3455 * ((U&0xff)-128) -
						0.7169*((V&0xff)-128));
        rgb.b = (int)((Y&0xff) + 1.779 * ((U&0xff)-128));
    }
    else
    {
        rgb.r = (int)((Y&0xff) + 1.732446 * ((U&0xff)-128));
        rgb.g = (int)((Y&0xff) - 0.698001 * ((U&0xff)-128) -
						0.703125*((V&0xff)-128));
        rgb.b = (int)((Y&0xff) + 1.370705 * ((V&0xff)-128));
    }

    rgb.r =(rgb.r<0? 0: rgb.r>255? 255 : rgb.r);
    rgb.g =(rgb.g<0? 0: rgb.g>255? 255 : rgb.g);
    rgb.b =(rgb.b<0? 0: rgb.b>255? 255 : rgb.b);

    return rgb;
}
/***********************************************************************************************/

// 以下是v4l2库封装函数
/***********************************************************************************************/
v4l2Agent * v4l2Agent::m_pSingleInstance = NULL;
/***********************************************************************************************/
v4l2Agent * v4l2Agent::GetInstance()
{
    if (NULL == m_pSingleInstance)
    {
        m_pSingleInstance = new v4l2Agent();
    }
    return m_pSingleInstance;
}
/***********************************************************************************************/
void v4l2Agent::ReleaseInstance()
{
    delete m_pSingleInstance;
    m_pSingleInstance = NULL;
}
/***********************************************************************************************/
v4l2Agent::v4l2Agent()
    : m_szdevName(NULL)
    , m_ioMethod(IO_METHOD_MMAP)
    , m_nfdCamera(-1)
    , m_buffers(NULL)
    , m_nbuffers(0)
    , m_szImage(NULL)
    , m_nImageSize(0)
    , m_bIsInit(false)
    , m_bIsStart(false)
{
    CLEAR(m_fmt);
	m_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	m_fmt.fmt.pix.width = 640;
	m_fmt.fmt.pix.height = 480;
    m_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	m_fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
}
/***********************************************************************************************/
v4l2Agent::~v4l2Agent()
{
    delete [] m_szdevName;
    m_szdevName = NULL;

    delete [] m_szImage;
    m_szImage = NULL;
}
/***********************************************************************************************/
int v4l2Agent::nInitCamera(const int & nImageWidth,
						   const int & nImageHeigt,
						   const char * szDevName,
                           int nDevNameSize,
						   const io_method io)
{
    int nRet = ERR_NONE;
    if (m_bIsInit)
    {
        return nRet;
    }

    m_szdevName = new char[260];
    memset(m_szdevName, '\0', 260);
    memcpy(m_szdevName, szDevName, nDevNameSize);
    m_fmt.fmt.pix.width = nImageWidth;
	m_fmt.fmt.pix.height = nImageHeigt;
    m_ioMethod = io;

    // 1.打开设备
    struct stat st;
	if (-1 == stat(m_szdevName, &st))
    {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                m_szdevName, errno,	strerror(errno));
        return ERR_IDENTIFY;
	}

	if (!S_ISCHR(st.st_mode))
    {
		fprintf(stderr, "%s is no device\n", m_szdevName);
        return ERR_NODEVICE;
	}

	m_nfdCamera = open(m_szdevName, O_RDWR /* required */| O_NONBLOCK, 0);

	if (-1 == m_nfdCamera)
    {
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
                m_szdevName, errno,	strerror(errno));
        return ERR_OPENFAILED;
	}

    nRet = innerInitDevice();
    if (ERR_NONE != nRet)
    {
        return nRet;
    }
    m_bIsInit = true;
    return nRet;
}
/***********************************************************************************************/
int v4l2Agent::innerInitDevice(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	unsigned int min;

    // 2.读取设备性能
	if (-1 == xioctl(m_nfdCamera, VIDIOC_QUERYCAP, &cap))
    {
		if (EINVAL == errno)
        {
			fprintf(stderr, "%s is no V4L2 device\n", m_szdevName);
            return ERR_NOV4L2DEV;
		}
        else
        {
            errno_exit("VIDIOC_QUERYCAP");
			return ERR_OTHER;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
		fprintf(stderr, "%s is no video capture device\n", m_szdevName);
        return ERR_NOVIDEOCAPTUREDEV;
	}

	switch (m_ioMethod)
    {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE))
        {
			fprintf(stderr, "%s does not support read i/o\n", m_szdevName);
            return ERR_NOSUPPORTREADIO;
		}

		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING))
        {
			fprintf(stderr, "%s does not support streaming i/o\n", m_szdevName);
            return ERR_NOSUPPORTSTREAMIO;
		}

		break;
	}

	/* Select video input, video standard and tune here. */

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    // 3.读取视频设备剪裁和缩放方面的能力
	if (0 == xioctl(m_nfdCamera, VIDIOC_CROPCAP, &cropcap))
    {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

        // 4.设定图像当前剪裁窗口
		if (-1 == xioctl(m_nfdCamera, VIDIOC_S_CROP, &crop))
        {
			switch (errno)
            {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	}
    else
    {
		/* Errors ignored. */
	}

    // 5.设定数据类型、数据格式及图像大小
	if (-1 == xioctl(m_nfdCamera, VIDIOC_S_FMT, &m_fmt))
    {
		errno_exit("VIDIOC_S_FMT");
        return ERR_OTHER;
    }

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = m_fmt.fmt.pix.width * 2;
	if (m_fmt.fmt.pix.bytesperline < min)
		m_fmt.fmt.pix.bytesperline = min;
	min = m_fmt.fmt.pix.bytesperline * m_fmt.fmt.pix.height;
	if (m_fmt.fmt.pix.sizeimage < min)
		m_fmt.fmt.pix.sizeimage = min;

	switch (m_ioMethod)
    {
	case IO_METHOD_READ:
		innerInitRead(m_fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		innerInitMMap();
		break;

	case IO_METHOD_USERPTR:
		innerInitUserPtr(m_fmt.fmt.pix.sizeimage);
		break;
	}

    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::innerInitRead(unsigned int buffer_size)
{
	m_buffers = (struct buffer *)calloc(1, sizeof(*m_buffers));

	if (!m_buffers) {
		fprintf(stderr, "Out of memory\n");
		return ERR_OUTMEMORY;
	}

	m_buffers[0].length = buffer_size;
	m_buffers[0].start = malloc(buffer_size);

	if (!m_buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		return ERR_OUTMEMORY;
	}

    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::innerInitMMap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

    // 6.初始化内存映射、分配缓冲区
	if (-1 == xioctl(m_nfdCamera, VIDIOC_REQBUFS, &req))
    {
		if (EINVAL == errno)
        {
			fprintf(stderr, "%s does not support "
					"memory mapping\n", m_szdevName);
            return ERR_NOSUPPORTMMAP;
		}
        else
        {
			errno_exit("VIDIOC_REQBUFS");
            return ERR_OTHER;
		}
	}

	if (req.count < 2)
    {
		fprintf(stderr, "Insufficient buffer memory on %s\n", m_szdevName);
		return ERR_INSUFFICIENTBUFFER;
	}

	m_buffers = (struct buffer *)calloc(req.count, sizeof(*m_buffers));

	if (!m_buffers)
    {
		fprintf(stderr, "Out of memory\n");
        return ERR_OUTMEMORY;
	}

	for (m_nbuffers = 0; m_nbuffers < req.count; ++m_nbuffers)
    {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = m_nbuffers;

        // 7.查询设备缓冲区大小、偏移地址等信息
		if (-1 == xioctl(m_nfdCamera, VIDIOC_QUERYBUF, &buf))
        {
			errno_exit("VIDIOC_QUERYBUF");
            return ERR_OTHER;
        }

		m_buffers[m_nbuffers].length = buf.length;

        // 8.映射设备缓冲区到应用程序控件的内存区
		m_buffers[m_nbuffers].start = mmap(NULL /* start anywhere */,
				buf.length,	PROT_READ | PROT_WRITE /* required */,
				MAP_SHARED /* recommended */, m_nfdCamera, buf.m.offset);

		if (MAP_FAILED == m_buffers[m_nbuffers].start)
        {
			errno_exit("mmap");
            return ERR_OTHER;
        }
	}

    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::innerInitUserPtr(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(m_nfdCamera, VIDIOC_REQBUFS, &req))
    {
		if (EINVAL == errno)
        {
			fprintf(stderr, "%s does not support "
					"user pointer i/o\n", m_szdevName);
			return ERR_NOSUPPORTUSERPOINTERIO;
		}
        else
        {
			errno_exit("VIDIOC_REQBUFS");
            return ERR_OTHER;
		}
	}

	m_buffers = (struct buffer *)calloc(4, sizeof(*m_buffers));

	if (!m_buffers)
    {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
        return ERR_OUTMEMORY;
	}

	for (m_nbuffers = 0; m_nbuffers < 4; ++m_nbuffers)
    {
		m_buffers[m_nbuffers].length = buffer_size;
		m_buffers[m_nbuffers].start = memalign(/* boundary */page_size,
				buffer_size);

		if (!m_buffers[m_nbuffers].start)
        {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
            return ERR_OUTMEMORY;
		}
	}
    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::nStartVedio(void)
{
    if (m_bIsStart)
    {
        return ERR_NONE;
    }

    if (!m_bIsInit)
    {
        return ERR_NOINIT;
    }

	enum v4l2_buf_type type;

	switch (m_ioMethod)
    {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (int i = 0; i < m_nbuffers; ++i)
        {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

            // 9.添加一个缓冲区到驱动程序的输入队列
			if (-1 == xioctl(m_nfdCamera, VIDIOC_QBUF, &buf))
            {
				errno_exit("VIDIOC_QBUF");
                return ERR_OTHER;
            }
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        // 10.启动流I/O，开始采集图像
		if (-1 == xioctl(m_nfdCamera, VIDIOC_STREAMON, &type))
        {
			errno_exit("VIDIOC_STREAMON");
            return ERR_OTHER;
        }

		break;

	case IO_METHOD_USERPTR:
		for (int i = 0; i < m_nbuffers; ++i)
        {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long) m_buffers[i].start;
			buf.length = m_buffers[i].length;

			if (-1 == xioctl(m_nfdCamera, VIDIOC_QBUF, &buf))
            {
				errno_exit("VIDIOC_QBUF");
                return ERR_OTHER;
            }
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(m_nfdCamera, VIDIOC_STREAMON, &type))
        {
			errno_exit("VIDIOC_STREAMON");
            return ERR_OTHER;
        }

		break;
	}
    m_bIsStart = true;
    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::nStopVedio(void)
{
    if (!m_bIsStart)
    {
        return ERR_NONE;
    }

    enum v4l2_buf_type type;

	switch (m_ioMethod)
    {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        // 15.停止流I/O，停止采集图像
		if (-1 == xioctl(m_nfdCamera, VIDIOC_STREAMOFF, &type))
        {
			errno_exit("VIDIOC_STREAMOFF");
            return ERR_OTHER;
        }

		break;
	}
    m_bIsStart = false;
    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::nGetYUYV422ImageSize(void)
{
    return (int)(m_fmt.fmt.pix.width * m_fmt.fmt.pix.height * 2);
}
/***********************************************************************************************/
int v4l2Agent::nGetCurrentYUYV422Image(BYTE *& szYUYV422Image)
{
    int nRet = ERR_NONE;

    if (!m_bIsStart)
    {
        return ERR_NOSTART;
    }

    nRet = innerGetImageLoop();
    if (ERR_NONE != nRet)
    {
        return nRet;
    }

    memcpy(szYUYV422Image, m_szImage, m_nImageSize);

    return nRet;
}
/***********************************************************************************************/
int v4l2Agent::nGetRGBOrBGRImageSize(void)
{
    return (int)(m_fmt.fmt.pix.width * m_fmt.fmt.pix.height * 3);
}
/***********************************************************************************************/
int v4l2Agent::nYUYV422ToRGB(const BYTE *& szYUYV422Image,
							 BYTE *& szRGBImage,
							 const int & nImageWidth,
							 const int & nImageHeigt)
{
    innerYUYV422ToRGBOrBGR(szYUYV422Image, szRGBImage,
							nImageWidth, nImageHeigt);
    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::nYUYV422ToBGR(const BYTE *& szYUYV422Image,
							 BYTE *& szBGRImage,
							 const int & nImageWidth,
							 const int & nImageHeigt)
{
    innerYUYV422ToRGBOrBGR(szYUYV422Image, szBGRImage,
							nImageWidth, nImageHeigt, false);
    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::innerYUYV422ToRGB(const BYTE *& szYUYV422,
								 BYTE *& szRGBImage,
								 const int & nYUYVSize)
{
    unsigned int in, out = 0;
    unsigned int pixel_16;
    BYTE pixel_24[3];
    unsigned int pixel32;
    int y0, u, y1, v;
    for(in = 0; in < nYUYVSize; in += 4) {
        pixel_16 = szYUYV422[in + 3] << 24 |
                   szYUYV422[in + 2] << 16 |
                   szYUYV422[in + 1] <<  8 |
                   szYUYV422[in + 0];
        y0 = (pixel_16 & 0x000000ff);
        u  = (pixel_16 & 0x0000ff00) >>  8;
        y1 = (pixel_16 & 0x00ff0000) >> 16;
        v  = (pixel_16 & 0xff000000) >> 24;
        pixel32 = convert_yuv_to_rgb_pixel(y0, u, v);
        pixel_24[0] = (pixel32 & 0x000000ff);
        pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
        pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
        szRGBImage[out++] = pixel_24[0];
        szRGBImage[out++] = pixel_24[1];
        szRGBImage[out++] = pixel_24[2];
        pixel32 = convert_yuv_to_rgb_pixel(y1, u, v);
        pixel_24[0] = (pixel32 & 0x000000ff);
        pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
        pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
        szRGBImage[out++] = pixel_24[0];
        szRGBImage[out++] = pixel_24[1];
        szRGBImage[out++] = pixel_24[2];
    }
    return ERR_NONE;
}
/***********************************************************************************************/
//YUY2是YUV422格式，排列是(YUYV)，是1 plane
int v4l2Agent::innerYUYV422ToRGBOrBGR(const BYTE *& szYUYV422Image,
									  BYTE *& szDesImage,
									  const int & nImageWidth,
									  const int & nImageHeigt,
									  const bool & bRGB)
{
    int lineWidth = 2 * nImageWidth;
    for(int i = 0; i < nImageHeigt; i++)
    {
        int startY = i * lineWidth;
        for(int j = 0; j < lineWidth; j += 4)
        {
            int Y1 = j + startY;
            int Y2 = Y1 + 2;
            int U = Y1 + 1;
            int V = Y1 + 3;
            int index = (Y1 >> 1) * 3;
            RGB tmp = yuvTorgbOrbgr(szYUYV422Image[Y1], szYUYV422Image[U],
										szYUYV422Image[V], bRGB);
            szDesImage[index + R] = tmp.r;
            szDesImage[index + G] = tmp.g;
            szDesImage[index + B] = tmp.b;
            index += 3;
            tmp = yuvTorgbOrbgr(szYUYV422Image[Y2], szYUYV422Image[U],
										szYUYV422Image[V], bRGB);
            szDesImage[index + R] = tmp.r;
            szDesImage[index + G] = tmp.g;
            szDesImage[index + B] = tmp.b;
        }
    }
    return ERR_NONE;
}
/**************************************************************************************/
//YUY2是YUV422格式，排列是(YUYV)，是1 plane
int v4l2Agent::nYUYV422ToGray(const BYTE *& szYUYV422Image,
							  BYTE *& szGrayImage,
							  const int & nImageWidth,
							  const int & nImageHeigt)
{
    int lineWidth = 2 * nImageWidth;
    int num = 0;
    for(int i = 0; i < nImageHeigt; i++)
    {
        int startY = i * lineWidth;
        for(int j = 0; j < lineWidth; j += 4)
        {
            int Y1 = j + startY;
            int Y2 = Y1 + 2;
            szGrayImage[num++] = szYUYV422Image[Y1];
            szGrayImage[num++] = szYUYV422Image[Y2];
        }
    }
    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::innerGetImageLoop(void)
{
	unsigned int count;

	//count = 100;
    count = 1;

	while (count-- > 0)
    {
		while (1)
        {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(m_nfdCamera, &fds);

			/* Timeout. */
			tv.tv_sec = 2;
			tv.tv_usec = 0;

            // 11.等待一帧图像采集完成
			r = select(m_nfdCamera + 1, &fds, NULL, NULL, &tv);

			if (-1 == r)
            {
				if (EINTR == errno)
					continue;

				errno_exit("select");
			}

			if (0 == r)
            {
				fprintf(stderr, "select timeout\n");
				exit(EXIT_FAILURE);
			}

			if (0 == innerReadFrame())
				break;

			/* EAGAIN - continue select loop. */
		}
	}

    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::innerReadFrame(void)
{
	struct v4l2_buffer buf;
	unsigned int i;

	switch (m_ioMethod)
    {
	case IO_METHOD_READ:
		if (-1 == read(m_nfdCamera, m_buffers[0].start, m_buffers[0].length))
        {
			switch (errno)
            {
			case EAGAIN:
				return ERR_OTHER;

			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				errno_exit("read");
                return ERR_OTHER;
			}
		}

		innerProcessImage(m_buffers[0].start, m_buffers[0].length);

		break;

	case IO_METHOD_MMAP:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

        // 12.从驱动程序的输出队列中取出一个缓冲区，得到一帧图像数据
		if (-1 == xioctl(m_nfdCamera, VIDIOC_DQBUF, &buf))
        {
			switch (errno)
            {
			case EAGAIN:
				return ERR_OTHER;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
                return ERR_OTHER;
			}
		}

		assert(buf.index < m_nbuffers);

        // 13.处理视频数据
		innerProcessImage(m_buffers[buf.index].start, buf.length);

        // 14.添加一个缓冲区到到驱动程序的输入队列
		if (-1 == xioctl(m_nfdCamera, VIDIOC_QBUF, &buf))
        {
			errno_exit("VIDIOC_QBUF");
            return ERR_OTHER;
        }

		break;

	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(m_nfdCamera, VIDIOC_DQBUF, &buf))
        {
			switch (errno)
            {
			case EAGAIN:
				return ERR_OTHER;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
                return ERR_OTHER;
			}
		}

		for (i = 0; i < m_nbuffers; ++i)
			if (buf.m.userptr == (unsigned long) m_buffers[i].start
					&& buf.length == m_buffers[i].length)
				break;

		assert(i < m_nbuffers);

		innerProcessImage((void *) buf.m.userptr, buf.length);

		if (-1 == xioctl(m_nfdCamera, VIDIOC_QBUF, &buf))
        {
			errno_exit("VIDIOC_QBUF");
            return ERR_OTHER;
        }

		break;
	}

	return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::innerProcessImage(const void * p, int size)
{
	if (m_szImage == NULL || m_nImageSize != size)
	{
		m_nImageSize = size;
		m_szImage = new char[size];
	    memset(m_szImage, '\0', size);
	}

    memcpy(m_szImage, p, size);

    return ERR_NONE;
}
/***********************************************************************************************/
int v4l2Agent::nUnInitCamera(void)
{
    if (!m_bIsInit)
    {
        return ERR_NONE;
    }

    if (m_bIsStart)
    {
        return ERR_START;
    }

    switch (m_ioMethod)
    {
	case IO_METHOD_READ:
		free(m_buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (int i = 0; i < m_nbuffers; ++i)
        {
            // 16.释放内存映射
			if (-1 == munmap(m_buffers[i].start, m_buffers[i].length))
				errno_exit("munmap");
        }
		break;

	case IO_METHOD_USERPTR:
		for (int i = 0; i < m_nbuffers; ++i)
			free(m_buffers[i].start);
		break;
	}

	free(m_buffers);

    if (-1 == close(m_nfdCamera))
    {
        // 17.关闭设备
		errno_exit("close");
        return ERR_OTHER;
    }
	m_nfdCamera = -1;
    m_bIsInit = false;
    return ERR_NONE;
}
/***********************************************************************************************/

//以下是对外导出函数
/***********************************************************************************************/
int nInitCamera(const int & nImageWidth,
                const int & nImageHeigt,
                const char * szDevName,
                int nDevNameSize,
                const io_method io)
{
    return V4L2->nInitCamera(nImageWidth, nImageHeigt, szDevName,nDevNameSize, io);
}
/***********************************************************************************************/
int nStartVedio(void)
{
    return V4L2->nStartVedio();
}
/***********************************************************************************************/
int nGetYUYV422ImageSize(void)
{
    return V4L2->nGetYUYV422ImageSize();
}
/***********************************************************************************************/
int nGetCurrentYUYV422Image(BYTE * szYUYV422Image)
{
	if (szYUYV422Image == NULL)
	{
		return ERR_MEMORY;
	}
    return V4L2->nGetCurrentYUYV422Image(szYUYV422Image);
}
/***********************************************************************************************/
int nYUYV422ToRGB(const BYTE * szYUYV422Image,
                  BYTE * szRGBImage,
                  const int & nImageWidth,
                  const int & nImageHeight)
{
	if (szRGBImage == NULL)
	{
		return ERR_MEMORY;
	}
    return V4L2->nYUYV422ToRGB(szYUYV422Image, szRGBImage,
								nImageWidth, nImageHeight);
}
/***********************************************************************************************/
int nYUYV422ToBGR(const BYTE * szYUYV422Image,
                  BYTE * szBGRImage,
                  const int & nImageWidth,
                  const int & nImageHeight)
{
	if (szBGRImage == NULL)
	{
		return ERR_MEMORY;
	}
    return V4L2->nYUYV422ToBGR(szYUYV422Image, szBGRImage,
								nImageWidth, nImageHeight);
}
/***********************************************************************************************/
int nYUYV422ToGray(const BYTE * szYUYV422Image,
                   BYTE * szGrayImage,
                   const int & nImageWidth,
                   const int & nImageHeight)
{
	if (szGrayImage == NULL)
	{
		return ERR_MEMORY;
	}
    return V4L2->nYUYV422ToGray(szYUYV422Image, szGrayImage,
								nImageWidth, nImageHeight);
}
/***********************************************************************************************/
int nGetRGBImageSize(void)
{
    return V4L2->nGetRGBOrBGRImageSize();
}
/***********************************************************************************************/
int nGetBGRImageSize(void)
{
    return V4L2->nGetRGBOrBGRImageSize();
}
/***********************************************************************************************/
int nStopVedio(void)
{
    return V4L2->nStopVedio();
}
/***********************************************************************************************/
int nUnInitCamera(void)
{
    return V4L2->nUnInitCamera();
}
/***********************************************************************************************/
