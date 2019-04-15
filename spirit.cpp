#include "h264decoder.h"

#define H264FRAMELEN 8
#define H264FRAMESIZE (512*1024)

bool H264Decoder::init()
{
    if (m_init)
        return true;

    m_init = true;
    if (!m_av_register)
    {
        m_av_register = true;
        av_register_all();
    }

    _pCodecContext = avcodec_alloc_context3(NULL);
    _pH264VideoDecoder = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (_pH264VideoDecoder == NULL)
    {
        printf("_pH264VideoDecoder null \n");
        return false;
    }

    //初始化参数，下面的参数应该由具体的业务决定  AV_PIX_FMT_YUV420P;
    _pCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    if (avcodec_open2(_pCodecContext, _pH264VideoDecoder, NULL) >= 0)
        _pFrameYuv = av_frame_alloc();
    else
    {
        printf("avcodec_open2 null \n");
        return false;
    }

    return true;
}

int H264Decoder::putVideoStream(uint8_t* buffer, int bufferLen)
{
//    uint8_t tst[27] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x28, 0xd9, 0x00,
//                       0x78, 0x02, 0x27, 0xe5, 0x84, 0x00, 0x00, 0x0f, 0xa4, 0x00,
//                       0x02, 0xee, 0x00, 0x3c, 0x60, 0xc9, 0x20};
    AVPacket packet = { 0 };
    packet.data = buffer;    //这里填入一个指向完整H264数据帧的指针
    packet.size = bufferLen;        //这个填入H264数据帧的大小

//    int xx=0;
//    for(int i=0;i<packet.size;i++)
//    {
//        xx = packet.data[i];
//        printf("0x%02x ", xx);
//    }
//    printf("\n");

    int ret = avcodec_send_packet(_pCodecContext, &packet);
    //printf("avcodec_send_packet %d\n", ret);
    return ret;
}

int H264Decoder::getNextVideoFrame(uint8_t* buffer, int bufferLen)
{
    int ret = avcodec_receive_frame(_pCodecContext, _pFrameYuv);
    //printf("avcodec_receive_frame %d\n", ret);
    if (ret == 0)
    {
        int height = _pCodecContext->height;
        int width = _pCodecContext->width;

        if (bufferLen < height * width * 3 / 2) return 2;
        ////写入数据  
        int yLen = height * width;
        memcpy(buffer, _pFrameYuv->data[0], yLen);

        int uLen = yLen / 4;
        memcpy(buffer + yLen, _pFrameYuv->data[1], uLen);

        int vLen = uLen;
        memcpy(buffer + yLen + uLen, _pFrameYuv->data[2], vLen);
        return 0;
    }
    return 1;
}
