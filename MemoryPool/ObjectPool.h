#pragma once

#include <queue>
#include <mutex>
#include <memory>
#include <functional>

/**
 * @brief 线程安全的对象池
 * 
 * 特点：
 * - 预分配对象，避免频繁的构造/析构
 * - 支持自定义初始化和重置函数
 * - 线程安全的 acquire/release
 * - 灵活的初始容量和最大容量配置
 * 
 * @tparam T 对象类型
 */
template<typename T>
class ObjectPool {
public:
    using ObjectPtr = std::unique_ptr<T, std::function<void(T*)>>;

    /**
     * @brief 构造对象池
     * @param initialSize 初始对象数量
     * @param maxSize 最大对象数量（0 表示无限制）
     * @param resetFunc 对象重置函数（从池中取出时调用）
     */
    ObjectPool(size_t initialSize = 100, size_t maxSize = 1000,
               std::function<void(T*)> resetFunc = nullptr)
        : m_initialSize(initialSize), m_maxSize(maxSize), m_resetFunc(resetFunc)
    {
        // 预分配初始对象
        for (size_t i = 0; i < initialSize; ++i) {
            m_pool.push(std::make_unique<T>());
        }
        m_totalCreated = initialSize;
    }

    virtual ~ObjectPool() = default;

    /**
     * @brief 从对象池中获取一个对象
     * @return 对象的智能指针，离开作用域时自动归还
     */
    ObjectPtr acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);

        T* obj = nullptr;
        if (!m_pool.empty()) {
            obj = m_pool.front().release();
            m_pool.pop();
        } else if (m_maxSize == 0 || m_totalCreated < m_maxSize) {
            // 如果池为空但未超出限制，创建新对象
            obj = new T();
            m_totalCreated++;
        } else {
            // 超出最大限制，返回 nullptr
            return ObjectPtr(nullptr, [this](T*) {});
        }

        // 重置对象状态
        if (obj && m_resetFunc) {
            m_resetFunc(obj);
        }

        // 返回智能指针，析构时调用 release()
        return ObjectPtr(obj, [this](T* ptr) {
            if (ptr) {
                this->release(ptr);
            }
        });
    }

    /**
     * @brief 获取当前池中空闲对象数量
     */
    size_t getFreeCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool.size();
    }

    /**
     * @brief 获取已创建的总对象数量
     */
    size_t getTotalCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_totalCreated;
    }

    /**
     * @brief 清空对象池
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_pool.empty()) {
            m_pool.pop();
        }
        m_totalCreated = 0;
    }

    /**
     * @brief 预热对象池（预分配更多对象）
     */
    void warmup(size_t count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < count; ++i) {
            if (m_maxSize == 0 || m_totalCreated < m_maxSize) {
                m_pool.push(std::make_unique<T>());
                m_totalCreated++;
            } else {
                break;
            }
        }
    }

protected:
    /**
     * @brief 将对象归还到池中
     */
    void release(T* obj) {
        if (!obj) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_pool.size() < m_initialSize * 2) {  // 避免池过大
            m_pool.push(std::unique_ptr<T>(obj));
        }
        // 否则让对象被销毁
    }

private:
    std::queue<std::unique_ptr<T>> m_pool;
    mutable std::mutex m_mutex;
    size_t m_initialSize;
    size_t m_maxSize;
    size_t m_totalCreated = 0;
    std::function<void(T*)> m_resetFunc;
};

/**
 * @brief 单例对象池管理器
 */
template<typename T>
class ObjectPoolManager {
public:
    static ObjectPoolManager& getInstance() {
        static ObjectPoolManager instance;
        return instance;
    }

    ObjectPool<T>& getPool() {
        return m_pool;
    }

private:
    ObjectPoolManager() = default;
    ~ObjectPoolManager() = default;

    ObjectPool<T> m_pool;
};
