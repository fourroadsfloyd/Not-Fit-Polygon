#include <cstdlib>
#include "ThreadCache.h"
#include "CentralCache.h"

namespace ysk
{

// ★ 定义全局计数器
// std::atomic<size_t> ThreadCache::g_global_cache_hits{0};
// std::atomic<size_t> ThreadCache::g_global_cache_misses{0};

ThreadCache::ThreadCache()
{
    freeList_.fill(nullptr);
    freeListSize_.fill(0);
}

void* ThreadCache::allocate(size_t size)
{
    size_t index = size == vertSize ? 0 : 1;

    void* ptr = freeList_[index];

    if(ptr != nullptr)
    {
        //更新头结点
        freeList_[index] = *reinterpret_cast<void**>(ptr);
        freeListSize_[index]--;
        //g_global_cache_hits.fetch_add(1, std::memory_order_relaxed);  // ★ 全局计数
        return ptr;
    }

    //g_global_cache_misses.fetch_add(1, std::memory_order_relaxed);  // ★ 全局计数
    return fetchFromCentralCache(index);

}

void* ThreadCache::fetchFromCentralCache(size_t index)
{
    size_t batchNum = 0;

    void* start = CentralCache::getInstance().fetchRange(index, batchNum);

    if (!start) return nullptr;

    void* result = start;

    // 将剩余的块放入线程本地自由链表
    if (batchNum > 1)
    {
        // 获取链表的第二个节点
        freeList_[index] = *reinterpret_cast<void**>(start);

        // 更新自由链表大小（减去返回给用户的 1 个）
        freeListSize_[index] += (batchNum - 1);
    }

    // 如果只有 1 个块，直接返回，不更新 freeList
    return result;
}

void ThreadCache::deallocate(void *ptr, size_t size)
{
    if(size > MAX_BYTES)    //系统分配的内存直接释放
    {
        free(ptr);
        return;
    }

    size_t index = size == vertSize ? 0 : 1;

    //将ptr插入到对应索引的空闲链表头部
    *reinterpret_cast<void**>(ptr) = freeList_[index];
    freeList_[index] = ptr;

    freeListSize_[index]++;

    // if(shouldReturnToCentralCache(index))
    // {
    //     returnToCentralCache(freeList_[index], size);
    // }

}

// bool ThreadCache::shouldReturnToCentralCache(size_t index)
// {
//     // 设定阈值，例如：当自由链表的大小超过一定数量时
//     size_t threshold = 512;
//     return (freeListSize_[index] > threshold);
// }

// void ThreadCache::returnToCentralCache(void *start, size_t size)
// {
//     //CentralCache::getInstance().returnRange(start, size, SizeClass::getIndex(size));

//     // 根据大小计算对应的索引
//     size_t index = SizeClass::getIndex(size);

//     // 获取对齐后的实际块大小
//     size_t alignedSize = SizeClass::ByteSize(index);

//     // 计算要归还内存块数量
//     size_t batchNum = freeListSize_[index];
//     if (batchNum <= 1) return; // 如果只有一个块，则不归还

//     // 保留一部分在ThreadCache中（比如保留1/4）
//     size_t keepNum = std::max(batchNum / 4, size_t(1));
//     size_t returnNum = batchNum - keepNum;

//     void* splitNode = start;
//     for (size_t i = 0; i < returnNum - 1; ++i)
//     {
//         splitNode = *reinterpret_cast<void**>(splitNode);
//     }

//     if (splitNode != nullptr)
//     {
//         // 将要返回的部分和要保留的部分断开
//         void* nextHead = *reinterpret_cast<void**>(splitNode);
//         *reinterpret_cast<void**>(splitNode) = nullptr; // 断开连接

//         // 更新ThreadCache的空闲链表
//         freeList_[index] = nextHead;

//         // 更新自由链表大小
//         freeListSize_[index] = keepNum;

//         // 将剩余部分返回给CentralCache
//         if (returnNum > 0 && start != nullptr)
//         {
//             //CentralCache::getInstance().returnRange(start, splitNode, index);
//         }
//     }
// }

}


