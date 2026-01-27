#pragma once

#include <map>
#include <mutex>
#include <QDebug>

namespace ysk
{

class PageCache
{
public:
    static const size_t PAGE_SIZE = 4 * 1024; // 4KB页面大小

    //单例模式中的懒汉式获取实例方法
    static PageCache& getInstance()
    {
        static PageCache instance;
        return instance;
    }

    void* allocateSpan(size_t numPages);

    // Release all allocated spans back to system
    void releaseAllSpanstoOS();

private:
    PageCache() = default;

    //单例模式中禁止拷贝构造和赋值运算符
    PageCache(const PageCache&) = delete;
    PageCache& operator=(const PageCache&) = delete;

    //当一次性申请的内存空间不够用时，将调用此函数从操作系统申请内存
    void* systemAlloc(size_t numPages);

private:
    //普通链表结构体，可以与centralCache和threadCache中的侵入式链表作对比
    struct Span
    {
        void* pageAddr;
        size_t numPages;
    };

    std::map<void*, Span*> spanMap_; //记录已分配的span
    std::mutex mutex_;

};

}
