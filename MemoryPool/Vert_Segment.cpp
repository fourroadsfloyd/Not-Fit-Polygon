#include "Vert_Segment.h"
#include "MemoryPool.h"

static bool g_use_mem_pool = true; 

//========================================================================
// Vert 的 operator new/delete 实现
//========================================================================
// 使用内存池分配
void* Vert::operator new(size_t size)
{
    if(g_use_mem_pool)
    {
        //g_vert_alloc_count.fetch_add(1, std::memory_order_relaxed);
        return ysk::MemoryPool::allocate(size);
    }

    else
        return ::operator new(size);
}

void Vert::operator delete(void* ptr, size_t size)
{
    if(g_use_mem_pool)
    //g_vert_delete_count.fetch_add(1, std::memory_order_relaxed);
        ysk::MemoryPool::deallocate(ptr, size);
    else
        ::operator delete(ptr);
}

//========================================================================
// Segment 的 operator new/delete 实现
//========================================================================

void* Segment::operator new(size_t size)
{
    if(g_use_mem_pool)
    //g_segment_alloc_count.fetch_add(1, std::memory_order_relaxed);
        return ysk::MemoryPool::allocate(size);
    else
        return ::operator new(size);
}

void Segment::operator delete(void* ptr, size_t size)
{
    if(g_use_mem_pool)
    //g_segment_delete_count.fetch_add(1, std::memory_order_relaxed);
        ysk::MemoryPool::deallocate(ptr, size);
    else
        ::operator delete(ptr);
}



