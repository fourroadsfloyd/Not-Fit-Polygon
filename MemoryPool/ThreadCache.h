#pragma once
#include <cstddef>
#include <array>
#include <atomic>
#include "Common.h"

namespace ysk
{

class ThreadCache{
private:
    ThreadCache();

    ThreadCache(const ThreadCache&) = delete;
    ThreadCache& operator=(const ThreadCache&) = delete;

    void* fetchFromCentralCache(size_t index);
    void returnToCentralCache(void* start, size_t size);
    bool shouldReturnToCentralCache(size_t index);
    size_t getBatchNum(size_t size);

    //编译器计算类型大小的静态常量表达式
    static constexpr size_t vertSize = sizeof(Vert);
    static constexpr size_t segmentSize = sizeof(Segment);

    // size_t vertSize = 0;
    // size_t segmentSize = 0;

public:
    static ThreadCache* getInstance()
    {
        static thread_local ThreadCache instance;
        return &instance;
    }

    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);
    
    // // ★ 全局统计计数器（所有线程累加）
    // static std::atomic<size_t> g_global_cache_hits;
    // static std::atomic<size_t> g_global_cache_misses;

private:

    //经典的侵入式空闲链表设计
    std::array<void*, FREE_LIST_SIZE> freeList_;      //每个索引对应一个空闲链表的头指针
    std::array<size_t, FREE_LIST_SIZE> freeListSize_; //每个索引对应当前空闲链表中的块数量
    
    // ★ 性能统计（已移到类级别全局变量）
    // std::atomic<size_t> allocCount_{0};
    // std::atomic<size_t> deallocCount_{0};
    // std::atomic<size_t> cacheHit_{0};      // 缓存命中次数
    // std::atomic<size_t> cacheMiss_{0};     // 缓存未命中次数

};

}
