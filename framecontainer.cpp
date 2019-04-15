#include "framecontainer.h"
#include <cassert>

#define LOCK_FOR_MILLTIME 5

FrameContainer::FrameContainer(int size, int num) :
    m_num(num),
    m_size(size)
{
    for (int i = 0; i < m_num; ++i)
    {
        SSmartData *data = new SSmartData;

        data->ptr = new uint8_t[m_size];
        data->sz = 0;
        data->status = SSmartData::E_DATA_FLAG::USED;
        data->yuvchanged = 0;

        m_ptrs.push_back(data);
    }
    
    m_decoder = new H264Decoder;
    bool ret = m_decoder->init();

    printf("init finished...[%d]\n", ret);

    printf("start preprocesser thread...\n");
    new std::thread(&FrameContainer::run, this);
}

FrameContainer::~FrameContainer()
{
    for (auto p: m_ptrs)
    {
        delete p->ptr;
        delete p;
    }
}

bool FrameContainer::produce(void* src, int len)
{
    assert(len <= m_size);

    SSmartData *ptr = m_ptrs[m_idxProducer];
    bool ok = ptr->lock.try_lock_for(std::chrono::milliseconds(LOCK_FOR_MILLTIME));
    //printf("%d\n", ok);
    //dump();
    bool skip = true;
    if (ok && ptr->status == SSmartData::E_DATA_FLAG::USED)
    {
        skip = false;

        ptr->sz = len;
        memcpy(ptr->ptr, src, len);//data

        //change status
        ptr->status = SSmartData::E_DATA_FLAG::ORIGIN;
        m_idxProducer = (m_idxProducer + 1) % m_num;
    }

    if (ok)
    {
        ptr->lock.unlock();
    }
    return !skip;
}

void FrameContainer::preprocesser()
{
    SSmartData *ptr = m_ptrs[m_idxPreprocesser];
    bool ok = ptr->lock.try_lock();
    if (!ok) return;

    if (ptr->status == SSmartData::E_DATA_FLAG::ORIGIN)
    {
        //parse
        parseSmartData(ptr);

        //change status
        ptr->status = SSmartData::E_DATA_FLAG::PROCESSED;
        m_idxPreprocesser = (m_idxPreprocesser + 1) % m_num;
    }
    ptr->lock.unlock();
}

const uint8_t * FrameContainer::consume(int *stat)
{
    const uint8_t *ret = nullptr;

    SSmartData *ptr = m_ptrs[m_idxConsumer];
    bool ok = ptr->lock.try_lock();
    if (!ok) return ret;

    if (ptr->status == SSmartData::E_DATA_FLAG::PROCESSED)
    {
        ret = ptr->ptr;
        *stat = ptr->yuvchanged;
    }
    else
    {
        ptr->lock.unlock();
    }
    return ret;
}

void FrameContainer::consumeFinished()
{
    SSmartData *ptr = m_ptrs[m_idxConsumer];
    //change status
    ptr->status = SSmartData::E_DATA_FLAG::USED;
    m_idxConsumer = (m_idxConsumer + 1) % m_num;
    ptr->lock.unlock();
}

//only be called from the UI(consumer thread), suppose producer/Preprocessor are stop before this.
void FrameContainer::clear()
{
    for (auto it : m_ptrs)
    {
        it->lock.try_lock_for(std::chrono::milliseconds(LOCK_FOR_MILLTIME));
        it->status = SSmartData::E_DATA_FLAG::USED;
        it->lock.unlock();
    }

    m_idxConsumer = 0;
    m_idxPreprocesser = 0;
    m_idxProducer = 0;
}

void FrameContainer::dump() const
{
    for (auto it : m_ptrs)
    {
        printf("%d ", it->status);
    }
    printf("\n%d %d %d\n\n", m_idxProducer, m_idxPreprocesser, m_idxConsumer);
}

void FrameContainer::parseSmartData(SSmartData* ps)
{
    //h264 decode
    auto t1 = std::chrono::steady_clock::now();
    m_decoder->putVideoStream(ps->ptr, ps->sz);
    auto t2 = std::chrono::steady_clock::now();
    int ret = m_decoder->getNextVideoFrame(ps->ptr, m_size);
    auto t3 = std::chrono::steady_clock::now();
    //printf("WASM msg size %d ,ret %d\n", ps->ptr, ret);
    if (!ret)
    {
        //printf("%f, %f\n",
        //    std::chrono::duration <double, std::milli>(t2 - t1).count(),
        //    std::chrono::duration <double, std::milli>(t3 - t2).count()
        //);
        ps->yuvchanged = 1;
    }
    else
    {
        ps->yuvchanged = 0;
    }
}

void FrameContainer::run()
{
    while (1)
    {
        preprocesser();
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
