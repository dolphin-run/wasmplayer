#ifndef FRAMECONTAINER_H
#define FRAMECONTAINER_H


#include <vector>
#include <thread>
#include <mutex>
#include <h264decoder.h>

struct SSmartData
{
    enum E_DATA_FLAG { USED, PROCESSED, ORIGIN };
    uint8_t* ptr;
    int sz;
    std::timed_mutex lock;
    E_DATA_FLAG status;

    int yuvchanged;
};

class FrameContainer
{
public:
    FrameContainer(int size, int num);
    virtual ~FrameContainer();

    bool produce(void* src, int len);
    void preprocesser();
    const uint8_t* consume(int *stat);
    void consumeFinished();
    void clear();
    void dump() const;

protected:
    void parseSmartData(SSmartData* ps);
    void run();

protected:
    int m_idxProducer = 0;
    int m_idxPreprocesser = 0;
    int m_idxConsumer = 0;

private:
    std::vector<SSmartData*> m_ptrs;
    int m_size = 0;
    int m_num = 0;

    H264Decoder *m_decoder;
};

#endif // FRAMECONTAINER_H