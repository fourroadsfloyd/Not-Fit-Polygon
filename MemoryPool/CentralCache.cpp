#include "CentralCache.h"
#include "PageCache.h"
#include "Vert_Segment.h"
#include "ThreadCache.h"
#include <QDebug>

namespace ysk
{

// 每次从PageCache获取span大小（以页为单位）
static const size_t Vert_Spans = VERT_NEED_SIZE_PER_THREAD / PageCache::PAGE_SIZE;
static const size_t Segment_Spans = SEGMENT_NEED_SIZE_PER_THREAD / PageCache::PAGE_SIZE;

CentralCache::CentralCache()
{
    // 初始化自旋锁
    for(auto& lock : locks_)
    {
        lock.clear(std::memory_order_relaxed);
    }
}

CentralCache::~CentralCache()
{
    // qDebug() << "\n============================================";
    // qDebug() << "=== MemoryPool Statistics ===";
    // qDebug() << "============================================";
    
    // qDebug() << "\n--- PageCache Statistics ---";
    // qDebug() << "Vert allocations from PageCache:" << vertcounter.load();
    // qDebug() << "Segment allocations from PageCache:" << segcounter.load();
    // qDebug() << "Vert batch size:" << (Vert_Spans * PageCache::PAGE_SIZE / vertSize) << "blocks per fetch";
    // qDebug() << "Segment batch size:" << (Segment_Spans * PageCache::PAGE_SIZE / segmentSize) << "blocks per fetch";
    // qDebug() << "Total Vert memory allocated:" << (vertcounter.load() * Vert_Spans * PageCache::PAGE_SIZE / (1024.0 * 1024.0)) << "MB";
    // qDebug() << "Total Segment memory allocated:" << (segcounter.load() * Segment_Spans * PageCache::PAGE_SIZE / (1024.0 * 1024.0)) << "MB";
    
    // qDebug() << "\n--- Vert/Segment Statistics ---";
    // size_t vert_alloc = Vert::g_vert_alloc_count.load();
    // size_t vert_delete = Vert::g_vert_delete_count.load();
    // size_t seg_alloc = Segment::g_segment_alloc_count.load();
    // size_t seg_delete = Segment::g_segment_delete_count.load();
    
    // qDebug() << "Total Vert allocations:" << vert_alloc;
    // qDebug() << "Total Vert deallocations:" << vert_delete;
    // qDebug() << "Total Segment allocations:" << seg_alloc;
    // qDebug() << "Total Segment deallocations:" << seg_delete;
    
    // qDebug() << "\n--- Memory Usage Estimation ---";
    // qDebug() << "Estimated Vert memory:" << (vert_alloc * sizeof(Vert) / (1024.0 * 1024.0)) << "MB";
    // qDebug() << "Estimated Segment memory:" << (seg_alloc * sizeof(Segment) / (1024.0 * 1024.0)) << "MB";
    // qDebug() << "Total estimated:" << ((vert_alloc * sizeof(Vert) + seg_alloc * sizeof(Segment)) / (1024.0 * 1024.0)) << "MB";
    
    // // ★ 缓存命中率统计（使用全局计数器）
    // size_t cacheHits = ThreadCache::g_global_cache_hits.load();
    // size_t cacheMisses = ThreadCache::g_global_cache_misses.load();
    // size_t totalAllocs = cacheHits + cacheMisses;
    // double hitRate = (totalAllocs > 0) ? (100.0 * cacheHits / totalAllocs) : 0.0;
    
    // qDebug() << "\n--- ThreadCache Performance ---";
    // qDebug() << "Cache hits:" << cacheHits;
    // qDebug() << "Cache misses:" << cacheMisses;
    // qDebug() << "Cache hit rate:" << QString::number(hitRate, 'f', 2) << "%";
    // qDebug() << "Total allocations from ThreadCache:" << totalAllocs;
    
    // qDebug() << "============================================\n";
}

void* CentralCache::fetchRange(size_t index, size_t& batchNum)
{
    // 索引检查，当索引大于等于FREE_LIST_SIZE时，说明申请内存过大应直接向系统申请
    if (index >= FREE_LIST_SIZE)
        return nullptr;

    SpinLocker locker(locks_[index]);   //RAII方式加锁

    size_t size = index == 0 ? vertSize : segmentSize;

    void* result = fetchFromPageCache(size);

    if (!result)
    {
        return nullptr;  // locker 析构时自动释放锁
    }

    // 将从PageCache获取的内存块切分成小块
    char* start = static_cast<char*>(result);   //char* 进行地址运算

    if(index == 0)
    {
        batchNum = (Vert_Spans * PageCache::PAGE_SIZE) / size;
    }
    else if(index == 1)
    {
        batchNum = (Segment_Spans * PageCache::PAGE_SIZE) / size;
    }

    // 构建返回给ThreadCache的内存块链表
    // 当allocBlocks == 1时，直接返回result即可，不需要构建链表
    if (batchNum > 1)
    {
        // 构建链表
        for (size_t i = 1; i < batchNum; ++i)
        {
            void* current = start + (i - 1) * size;
            void* next = start + i * size;
            *reinterpret_cast<void**>(current) = next;
        }
        //最后一个块指向nullptr
        *reinterpret_cast<void**>(start + (batchNum - 1) * size) = nullptr;
    }

    return result;
}

void* CentralCache::fetchFromPageCache(size_t size)
{
    if(size == vertSize)
    {
        //vertcounter.fetch_add(1,std::memory_order_relaxed);
        return PageCache::getInstance().allocateSpan(Vert_Spans);
    }

    if(size == segmentSize)
    {
        //segcounter.fetch_add(1,std::memory_order_relaxed);
        return PageCache::getInstance().allocateSpan(Segment_Spans);
    }

    return nullptr;
}


}
