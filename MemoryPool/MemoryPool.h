#pragma once

#include "ThreadCache.h"
#include <cstddef>

/*
该内存池属于短时间一次性内存池，为了解决并发任务中频繁分配和释放大量小对象的性能问题而设计。
ThreadCache类实现了线程本地缓存，每个线程都有自己的ThreadCache实例，用于管理小对象的分配和释放。
当线程需要分配内存时，首先从自己的ThreadCache中查找是否有可用的内存块。
如果有，则直接返回该内存块；
如果没有，则从CentralCache中批量获取
获取的时候也是批量获取，以减少与CentralCache的交互次数，从而提高性能。
当线程释放内存时，内存块会被放回到ThreadCache中，以便下次分配时可以直接使用。
如果ThreadCache中的内存块过多，不考虑归还给CentralCache以简化实现。
这种设计大大减少了线程间的锁竞争，提高了多线程环境下小对象分配和释放的效率，适用于需要频繁创建和销毁大量小对象的场景。
同样CentralCache类实现了中心缓存，负责管理所有线程共享的内存资源。
当ThreadCache需要更多内存时，会向CentralCache请求内存块。
CentralCache从PageCache中获取大块内存，然后将其切分成小块，返回给ThreadCache。
CentralCache仅是将span划分为内存块，其本身不管理任何内存。
这样直接从PageCache获取大块内存，减少了与操作系统交互的次数，提高了内存分配的效率。
*/

namespace ysk
{

class MemoryPool{
public:
    static void* allocate(size_t size)
    {
        return ThreadCache::getInstance()->allocate(size);
    }

    static void deallocate(void* ptr, size_t size)
    {
        ThreadCache::getInstance()->deallocate(ptr, size);
    }
};

}
