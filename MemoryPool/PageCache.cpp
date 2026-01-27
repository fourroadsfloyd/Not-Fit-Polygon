#include "PageCache.h"
#include <cstring>

#ifdef _WIN32
    #include <Windows.h>
    #include <memoryapi.h>
#else
    #include <sys/mman.h>
#endif

namespace ysk
{

void *PageCache::allocateSpan(size_t numPages)
{
    std::lock_guard<std::mutex> lock(mutex_);

    //freeSpans_中没有合适的span,需要向操作系统申请内存
    void* memory = systemAlloc(numPages);
    if(!memory)
    {
        return nullptr;
    }

    Span* span = new Span();    //创建新的span记录系统分配的内存
    span->pageAddr = memory;
    span->numPages = numPages;

    //记录已分配的span，不再放入freeSpans_中
    spanMap_[span->pageAddr] = span;

    return span->pageAddr;
}

void *PageCache::systemAlloc(size_t numPages)
{
    size_t size = numPages * PageCache::PAGE_SIZE;

#ifdef _WIN32
    void* ptr = VirtualAlloc(NULL, size, MEM_COMMIT |
                                             MEM_RESERVE, PAGE_READWRITE);

    if(ptr == nullptr)
    {
        return nullptr;
    }
#else
    void* ptr = mmap(nullptr, size, PROT_READ |
                                        PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(ptr == MAP_FAILED)
    {
        return nullptr;
    }
#endif

    //memset(ptr, 0, size);

    return ptr;
}

void PageCache::releaseAllSpanstoOS()
{
    std::lock_guard<std::mutex> lock(mutex_);

    for(auto& pair : spanMap_)
    {
        Span* span = pair.second;
        size_t size = span->numPages * PageCache::PAGE_SIZE;

// Release memory to OS
#ifdef _WIN32
        VirtualFree(span->pageAddr, 0, MEM_RELEASE);
#else
        munmap(span->pageAddr, size);
#endif

        delete span;
    }
    spanMap_.clear();
}


}
