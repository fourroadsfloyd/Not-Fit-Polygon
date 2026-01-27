#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <cassert>
#include <vector>
#include <atomic>
#include <chrono>
#include <QDebug>

template<typename T>
class ObjectPool {
public:
    // ✅ 全局初始化计数器（所有线程共享）
    // static std::atomic<size_t>& getInitCount() {
    //     static std::atomic<size_t> init_count{0};
    //     return init_count;
    // }

    static ObjectPool& getInstance()
    {
        static thread_local ObjectPool instance;
        return instance;
    }

    void initialize(size_t pool_size)
    {
        if (_initialized) {
            return;
        }

        // ✅ 记录初始化开始时间
        //auto start = std::chrono::high_resolution_clock::now();
        
        _free_objs.reserve(pool_size);  // 预分配容量

        for(size_t i = 0; i < pool_size; ++i)
        {
            _free_objs.push_back(new T());
        }
        _pool_size = pool_size;
        _initialized = true;
        
        // ✅ 统计初始化次数和耗时
        //getInitCount().fetch_add(1, std::memory_order_relaxed);
        
        // auto end = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        // qDebug() << "线程" << QThread::currentThreadId()
        //          << "对象池初始化完成，大小:" << pool_size
        //          << "耗时:" << duration << "ms"
        //          << "全局初始化次数:" << getInitCount().load();
    }

    template <typename... Args>
    T* acquire(Args&&... args)
    {
        if(_free_objs.empty() == false)
        {
            T* obj = _free_objs.back();
            _free_objs.pop_back();
            init_if_possible(obj, std::forward<Args>(args)...);
            return obj;
        }
        else
        {
            T* obj = new T();
            init_if_possible(obj, std::forward<Args>(args)...);
            return obj;
        }
    }

    void release(T* obj)
    {
        if(obj == nullptr) return;
        reset_if_possible(obj);
        _free_objs.push_back(obj);
    }

private:
    ObjectPool(size_t pool_size = 100)
        : _pool_size(pool_size),_initialized(false)
    {

    }

    ObjectPool(const ObjectPool&)            = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ~ObjectPool()
    {
        for(auto& obj : _free_objs)
        {
            delete obj;
        }
    }

private:

    //判断类中是否存在某个函数   SFINAE技术（替换失败不是错误）
    template <typename U, typename... Args>
    static auto init_if_possible(U* p, Args&&... args) -> decltype(p->init(std::forward<Args>(args)...), void()) {
        p->init(std::forward<Args>(args)...);
    }
    static void init_if_possible(...) { /* T 没有 reset，noop */ }

    template <typename U>
    static auto reset_if_possible(U* p) -> decltype(p->reset(), void()) {
        p->reset();
    }
    static void reset_if_possible(...) { /* T 没有 reset，noop */ }

private:
    size_t _pool_size;
    std::vector<T*> _free_objs;
    bool _initialized = false;
};


#endif // OBJECTPOOL_H
