#pragma once

#include <thread>
#include <atomic>
#include <array>
#include "Vert_Segment.h"
#include "PageCache.h"
#include "ThreadCache.h"

namespace ysk
{

struct SpanTracker{

    std::atomic<void*> spanAddr{nullptr};

    std::atomic<size_t> numPages{0};

    std::atomic<size_t> blockCount{0};

    std::atomic<size_t> freeCount{0};
};

class SpinLocker{
public:
    SpinLocker(std::atomic_flag& lock) : lock_(lock)
    {
        while (lock_.test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield(); // 添加线程让步，避免忙等待，避免过度消耗CPU
        }
    }

    ~SpinLocker()
    {
        lock_.clear(std::memory_order_release);
    }

    SpinLocker(const SpinLocker&) = delete;
    SpinLocker& operator=(const SpinLocker&) = delete;

private:
    std::atomic_flag& lock_;
};

class CentralCache{
private:
    CentralCache();

    ~CentralCache();
    CentralCache(const CentralCache&) = delete;
    CentralCache& operator=(const CentralCache&) = delete;

    void* fetchFromPageCache(size_t size);

    //编译器计算类型大小的静态常量表达式
    static constexpr size_t vertSize = sizeof(Vert);
    static constexpr size_t segmentSize = sizeof(Segment);

public:
    static CentralCache& getInstance()
    {
        static CentralCache instance;
        return instance;
    }

    void* fetchRange(size_t index,size_t& batchNum);

    void returnBlock(void* start, void* end, size_t index);

    // std::atomic<size_t> vertcounter{0};
    // std::atomic<size_t> segcounter{0};

private:

    std::array<std::atomic_flag, FREE_LIST_SIZE> locks_;    //自旋锁
    
    // 统计计数器
    // std::atomic<size_t> vertcounter{0};
    // std::atomic<size_t> segcounter{0};
};

}
