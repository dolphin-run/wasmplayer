#pragma once

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
//#include "libswscale/swscale.h"
//#include "libavutil/avutil.h"
};

class H264Decoder
{
public:
    bool init();
    int putVideoStream(uint8_t* buffer, int bufferLen);
    int getNextVideoFrame(uint8_t* buffer, int bufferLen);

private:
    bool m_init = false;
    bool m_av_register = false;
    AVCodecContext *_pCodecContext = nullptr;
    AVCodec *_pH264VideoDecoder = nullptr;
    AVFrame *_pFrameYuv = nullptr;
};
