## 一、内存池架构设计技术

### 1.1 三层分层架构 ([MemoryPool.h:26-37](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/MemoryPool.h))



```
ThreadCache (L1) → CentralCache (L2) → PageCache (L3)
     ↓                    ↓                    ↓
  线程本地             中心缓存              页级管理
  无锁访问             自旋锁保护            系统调用
```

**设计理念**：将不同大小的内存请求在不同层次处理，平衡速度与效率

### 1.2 大小分类策略 ([Common.h:27-30](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/Common.h))



```cpp
constexpr size_t ALIGNMENT = sizeof(Segment);  // 对齐边界
constexpr size_t MAX_BYTES = 256 * 1024;       // 最大缓存字节数
constexpr size_t FREE_LIST_SIZE = 2;           // 仅2种大小：Vert和Segment
```

**简化设计**：针对特定场景优化，只管理Vert和Segment两种固定大小

------

## 二、ThreadCache 层技术点

### 2.1 thread_local 线程本地存储 ([ThreadCache.h:30-34](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/ThreadCache.h))



```cpp
static ThreadCache* getInstance() {
    static thread_local ThreadCache instance;  // 每个线程独立实例
    return &instance;
}
```

**技术优势**：

- 零锁开销：线程间完全隔离，无需同步
- 缓存友好：CPU缓存命中率更高
- 伪共享避免：不同线程数据在不同缓存行

### 2.2 侵入式空闲链表 ([ThreadCache.cpp:19-28](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/ThreadCache.cpp))



```cpp
void* ptr = freeList_[index];
if(ptr != nullptr) {
    // 对象前几个字节存储next指针
    freeList_[index] = *reinterpret_cast<void**>(ptr);
    freeListSize_[index]--;
    return ptr;
}
```

**关键技术**：

- **零额外开销**：利用已释放对象本身的空间存储指针
- **缓存友好**：指针和数据在同一内存区域
- **经典实现**：Linux内核 slab 分配器采用同样技术

### 2.3 批量分配策略 ([ThreadCache.cpp:38-60](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/ThreadCache.cpp))



```cpp
void* fetchFromCentralCache(size_t index) {
    size_t batchNum = 0;
    void* start = CentralCache::getInstance().fetchRange(index, batchNum);
    
    void* result = start;  // 返回第一个
    
    if (batchNum > 1) {
        // 剩余的存入ThreadCache
        freeList_[index] = *reinterpret_cast<void**>(start);
        freeListSize_[index] += (batchNum - 1);
    }
    return result;
}
```

**技术优势**：减少与CentralCache的交互次数，降低锁竞争

------

## 三、CentralCache 层技术点

### 3.1 RAII 自旋锁 ([CentralCache.h:24-44](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/CentralCache.h))



```cpp
class SpinLocker {
public:
    SpinLocker(std::atomic_flag& lock) : lock_(lock) {
        while (lock_.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();  // 让出CPU，避免忙等待
        }
    }
    
    ~SpinLocker() {
        lock_.clear(std::memory_order_release);
    }
};
```

**技术要点**：

- **test_and_set**：原子操作，保证线程安全
- **yield()**：避免空转浪费CPU
- **RAII**：异常安全，自动释放锁

### 3.2 按索引分桶锁 ([CentralCache.h:76](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/CentralCache.h))



```cpp
std::array<std::atomic_flag, FREE_LIST_SIZE> locks_;  // 每个大小一把锁
```

**技术优势**：不同大小的内存请求互不干扰，提高并发度

### 3.3 链表构建技术 ([CentralCache.cpp:99-110](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/CentralCache.cpp))



```cpp
if (batchNum > 1) {
    // 构建单向链表
    for (size_t i = 1; i < batchNum; ++i) {
        void* current = start + (i - 1) * size;
        void* next = start + i * size;
        *reinterpret_cast<void**>(current) = next;  // 链接节点
    }
    *reinterpret_cast<void**>(start + (batchNum - 1) * size) = nullptr;  // 尾节点
}
```

**技巧**：利用指针算术，在连续内存块上构建链表结构

------

## 四、PageCache 层技术点

### 4.1 页大小对齐 ([PageCache.h:13](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/PageCache.h))



```cpp
static const size_t PAGE_SIZE = 4 * 1024;  // 4KB，与操作系统页大小一致
```

**技术意义**：

- 与系统内存管理单元对齐
- 减少内存碎片
- 提高内存访问效率

### 4.2 Span 管理 ([PageCache.h:39-43](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/PageCache.h))



```cpp
struct Span {
    void* pageAddr;   // 起始地址
    size_t numPages;  // 页数
};

std::map<void*, Span*> spanMap_;  // 红黑树管理
```

**数据结构选择**：map 提供 O(log n) 查找，有序遍历

------

## 五、对象池技术点

### 5.1 模板特化与 SFINAE ([ObjectPool.h:100-111](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/objectpool/objectpool.h))



```cpp
// 如果T有init(Args...)，调用它
template<typename U, typename... Args>
static auto init_if_possible(U* p, Args&&... args) 
    -> decltype(p->init(std::forward<Args>(args)...), void()) {
    p->init(std::forward<Args>(args)...);
}
static void init_if_possible(...) { /*noop*/ }

// 如果T有reset()，调用它
template<typename U>
static auto reset_if_possible(U* p) 
    -> decltype(p->reset(), void()) {
    p->reset();
}
```

**技术原理**：

- **SFINAE**（Substitution Failure Is Not An Error）
- **decltype** 类型推导
- **逗号表达式**：`p->init(...), void()` 返回 void 类型
- **重载决议**：优先匹配有init/reset的版本

### 5.2 完美转发 ([ObjectPool.h:55-70](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/objectpool/objectpool.h))



```cpp
template<typename... Args>
T* acquire(Args&&... args) {
    // ...
    init_if_possible(obj, std::forward<Args>(args)...);
    return obj;
}
```

**技术要点**：

- **万能引用** `Args&&`：绑定左值或右值
- **std::forward**：保持原始值类别
- **可变参数模板**：支持任意数量参数

### 5.3 thread_local 单例 ([ObjectPool.h:19-23](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/objectpool/objectpool.h))



```cpp
static ObjectPool& getInstance() {
    static thread_local ObjectPool instance;  // 每线程独立
    return instance;
}
```

**优势**：

- 线程安全，无需锁
- 延迟初始化
- 自动析构

------

## 六、C++ 内存管理底层技术

### 6.1 operator new/delete 重载 ([Vert_Segment.cpp:10-51](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/Vert_Segment.cpp))



```cpp
void* Vert::operator new(size_t size) {
    if(g_use_mem_pool)
        return ysk::MemoryPool::allocate(size);
    else
        return ::operator new(size);  // 全局operator new
}

void Vert::operator delete(void* ptr, size_t size) {
    if(g_use_mem_pool)
        ysk::MemoryPool::deallocate(ptr, size);
    else
        ::operator delete(ptr);
}
```

**技术点**：

- **类特定 new/delete**：覆盖全局行为
- **size_t 参数**：delete 时知道对象大小
- **运行时切换**：通过 `g_use_mem_pool` 选择策略

### 6.2 placement new（隐式使用）

在对象池中，`acquire` 返回的对象可能使用 placement new 构造：



```cpp
T* obj = _free_objs.back();
_free_objs.pop_back();
init_if_possible(obj, ...);  // 相当于重新构造
```

------

## 七、内存对齐与数值计算技术

### 7.1 对齐策略 ([Common.h:28](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/Common.h))



```cpp
constexpr size_t ALIGNMENT = sizeof(Segment);
```

**技术原理**：

- 按最大对象大小对齐
- 避免跨缓存行访问
- 满足CPU对齐要求

### 7.2 位运算技巧 ([SizeClass.h:45-47](vscode-webview://0oks11rrao8favd594h7e3rq32hm3b2m8phfbv5h7oq6q0q596sq/MemoryPool/Common.h))



```cpp
static FORCE_INLINE size_t roundUp(size_t bytes) {
    return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);  // 向上对齐
}
```

**位运算分析**：

- `ALIGNMENT - 1`：对齐掩码
- `~(ALIGNMENT - 1)`：清除低位
- `&` 操作：向下对齐到ALIGNMENT边界

------

## 八、并发控制技术总结

| 技术点               | 实现位置     | 作用           |
| -------------------- | ------------ | -------------- |
| thread_local         | ThreadCache  | 线程隔离，零锁 |
| atomic_flag          | CentralCache | 自旋锁原语     |
| memory_order_acquire | SpinLocker   | 获取内存屏障   |
| memory_order_release | SpinLocker   | 释放内存屏障   |
| yield()              | SpinLocker   | 避免忙等待     |
| mutex                | PageCache    | 互斥锁         |

------

## 九、设计模式应用

1. **单例模式**：CentralCache、PageCache、ObjectPool
2. **RAII**：SpinLocker 自动锁管理
3. **策略模式**：g_use_mem_pool 切换内存分配策略
4. **对象池模式**：复用Vert/Segment对象

------

## 十、性能优化技术

1. **空间换时间**：预分配内存池
2. **批量操作**：减少系统调用和锁竞争
3. **缓存友好**：thread_local 提高局部性
4. **无锁设计**：ThreadCache 层避免同步开销
5. **零开销抽象**：模板编译期展开





# 扫描线中两个比较器详细分析

## 一、EventComp - 事件队列比较器

### 1.1 用途



```cpp
using EventQue = std::priority_queue<Event, std::vector<Event>, EventComp>;
```

**作用**：定义事件在优先级队列中的弹出顺序。`priority_queue` 是大顶堆，`operator()` 返回 `true` 表示 A 应该排在 B 后面（B 优先弹出）。

### 1.2 完整逻辑解析



```cpp
bool operator()(const Event& A, const Event& B) const
```

#### **第一层：x 坐标优先**



```cpp
if (A.v->pt.x != B.v->pt.x)
    return A.v->pt.x > B.v->pt.x; // x小的优先弹出
```

**含义**：扫描线从左向右移动，x 坐标小的事件先处理。

------

#### **第二层：都是垂直线段 + 都是 LEFT 事件**



```cpp
if(!A.seg->existlk && !B.seg->existlk && A.type == 0 && B.type == 0)
{
    return A.v->pt.y > B.v->pt.y;  // y小的先弹出
}
```

**含义**：两条垂直线段的左端点在同一 x 位置，按 y 从小到大处理。

**图示**：



```
    ↓ 处理顺序
    B(5, 10) ← 先处理
    A(5, 20) ← 后处理
    ──────────────
```

------

#### **第三层：A 是垂直线段 + A 是 LEFT 事件**



```cpp
if(!A.seg->existlk && A.type == 0)
{
    return false;  // A先弹出
}
```

**含义**：A（垂直 LEFT）比 B 优先，返回 `false` 表示 A 不排在 B 后面。

------

#### **第四层：B 是垂直线段 + B 是 LEFT 事件**



```cpp
if(!B.seg->existlk && B.type == 0)
{
    return true;  // B先弹出
}
```

**含义**：B（垂直 LEFT）比 A 优先，返回 `true` 表示 A 排在 B 后面。

------

#### **第五层：都是垂直线段 + 都是 RIGHT 事件**



```cpp
if(!A.seg->existlk && !B.seg->existlk && A.type == 1 && B.type == 1)
{
    return A.v->pt.y > B.v->pt.y;  // y小的先弹出
}
```

**含义**：两条垂直线段的右端点在同一 x 位置，按 y 从小到大处理。

------

#### **第六层：A 是垂直线段 + A 是 RIGHT 事件**



```cpp
if(!A.seg->existlk && A.type == 1)
{
    return true;  // B先弹出（即非垂直的先弹出）
}
```

**含义**：垂直线段的 RIGHT 事件要让位于非垂直线段。

------

#### **第七层：B 是垂直线段 + B 是 RIGHT 事件**



```cpp
if(!B.seg->existlk && B.type == 1)
{
    return false;  // A先弹出（即非垂直的先弹出）
}
```

**含义**：同上，垂直 RIGHT 优先级最低。

------

#### **第八层：通用规则 - LEFT 优先于 RIGHT**



```cpp
if (A.type != B.type)
    return A.type < B.type;  // left(0) < right(1)，LEFT优先
```

**注释说明**（代码第 79-81 行）：

> "扫描线左边的线段只有与当前扫描线位置的垂直线段存在交点，非垂直不可能存在交点。比较 type 前，已经先让垂直线段进入了 set，这样避免了漏判交点。"

**核心思想**：LEFT 事件先处理，让垂直线段先进入状态结构，确保能检测到交点。

------

#### **第九层：y 坐标决胜**



```cpp
return A.v->pt.y > B.v->pt.y;  // y小的先弹出
```

**含义**：当所有其他条件都相同时，按 y 坐标升序处理。

------

### 1.3 优先级总结（从高到低）

| 条件               | 优先级                      |
| ------------------ | --------------------------- |
| x 更小             | 最高                        |
| 都是垂直 LEFT      | y 小的优先                  |
| 垂直 LEFT vs 其他  | 垂直 LEFT 优先              |
| 都是垂直 RIGHT     | y 小的优先                  |
| 垂直 RIGHT vs 其他 | 其他优先（垂直 RIGHT 段后） |
| LEFT vs RIGHT      | LEFT 优先                   |
| y 更小             | 决胜                        |

------

## 二、SegmentComparator - 状态结构线段比较器

### 2.1 用途



```cpp
std::set<Segment*, SegmentComparator> status;
```

**作用**：定义线段在状态结构中的排序（从下到上，y 值从小到大）。

### 2.2 核心挑战：动态排序

状态结构中的线段顺序**随着扫描线移动而变化**：



```
扫描线在不同位置，线段的相对上下关系可能改变：

     x = 10          x = 20          x = 30
        │              │              │
    ───┼──── s1     s1 ───────    ──────── s1
      s2 ──────      ───┼──── s2   s2 ───────
        │              │              │

在 x=10: s1 在上
在 x=20: s2 在上
在 x=30: s1 在上
```

因此需要一个**指向 currentX 的指针**：



```cpp
int64* currentX;  // 指向当前扫描线位置
SegmentComparator(int64* x) : currentX(x) {}
```

------

### 2.3 完整逻辑解析

#### **getYAtX 函数：计算线段在 x 处的 y 值**



```cpp
inline double getYAtX(const Segment& s, int64 x) const
{
    if (s.existlk)  // 非垂直线段
    {
        return (double)(s.lk * x + s.lb);  // y = kx + b
    }
    
    // 垂直线段：返回较小的 y 值（下端点）
    return (double)(std::min(s.a->pt.y, s.b->pt.y));
}
```

**关键点**：垂直线段返回**下端点的 y**，这样在 set 中排在较低位置。

------

#### **主比较函数**



```cpp
bool operator()(Segment* a, Segment* b) const {
    double ya = getYAtX(*a, *currentX);
    double yb = getYAtX(*b, *currentX);
    
    // 情况1：都是垂直线段
    if(!a->existlk && !b->existlk)
    {
        return ya < yb;  // 直接比较 y 值
    }
    
    // 情况2：y 值接近（可能是同一位置或数值误差）
    if(std::fabs(ya - yb) < 3)
    {
        // 特殊处理：确保垂直线段排在下面
        if(!a->existlk)
        {
            return false;  // a(垂直) 应该排在 b 下面，a < b 返回 false
        }
        
        if(!b->existlk)
        {
            return true;   // b(垂直) 应该排在 a 下面，a < b 返回 true
        }
        
        // 都不垂直：用 x+1 位置重新计算，避免数值误差
        ya = getYAtX(*a, *currentX + 1);
        yb = getYAtX(*b, *currentX + 1);
    }
    
    return ya < yb;  // 正常情况：y 小的排在前面
}
```

------

### 2.4 逻辑流程图



```
                ┌─────────────────────┐
                │  计算 ya, yb        │
                └──────────┬──────────┘
                           │
                           ▼
                ┌─────────────────────┐
                │ 都是垂直线段？       │
                └──────────┬──────────┘
                    是 │           │ 否
                       ▼           ▼
              ┌─────────────┐   ┌─────────────────┐
              │ ya < yb     │   │ |ya - yb| < 3?  │
              └─────────────┘   └────────┬────────┘
                                       │
                          是 │                   │ 否
                             ▼                   ▼
                    ┌───────────────┐     ┌─────────────┐
                    │ a垂直？       │     │  ya < yb    │
                    └───────┬───────┘     └─────────────┘
                     是 │    │ 否
                        ▼    ▼
              ┌──────────┐ ┌────────────────────┐
              │false     │ │ 用 x+1 重新计算    │
              │(a在下层) │ └─────────┬──────────┘
              └──────────┘           │
                                    ▼
                          ┌─────────────────────┐
                          │  ya_new < yb_new    │
                          └─────────────────────┘
```

------

### 2.5 垂直线段特殊处理的必要性

**问题场景**：当垂直线段与斜线在某个 x 位置"相遇"时



```
         x = 100
            │
        ────┼──── 斜线 s1 (y = 0.01x + 50) → ya ≈ 51
            │
        垂直线段 s2 (x = 100, y ∈ [40, 60])
            │

如果 ya = 51, yb = 51（数值接近）
如果不特殊处理，顺序可能不稳定
```

**解决方案**：

1. 垂直线段总是排在非垂直线段下面（`a->existlk == false` 时返回 `false`）
2. 确保垂直线段能作为"基准"，其他线段与之比较交点

------

## 三、两个比较器的协同工作

### 3.1 事件队列决定处理顺序



```
优先队列弹出顺序
    ↓
LEFT 事件 → handleLeftEvent() → 线段插入 status
    ↓
RIGHT 事件 → handleRightEvent() → 线段从 status 移除
```

### 3.2 状态结构维护相对位置



```
status (set) 中线段从下到上排列：
    ↓
   s3 ─────────  y 值最大
   s2 ─────────  
   s1 ─────────  y 值最小

插入新线段时，checkNeighbours() 检查与上下邻居的交点
```

### 3.3 垂直线段处理流程

**为什么垂直线段的 LEFT 事件要优先？**



```
假设有两条线段：
- 垂直线段 v: x = 100, y ∈ [0, 100]
- 斜线 s: 从 (50, 50) 到 (150, 60)

正确流程：
1. 处理 v 的 LEFT (100, 0)  → v 插入 status
2. 处理 s 的 LEFT (50, 50)  → s 插入 status
3. 此时 status 中: s 在 v 下面（因为 x=50 时 s.y=50 < v.y=0）
4. 扫描线移动到 x=100 附近
5. 检测到 s 与 v 的交点 (100, 55)

如果 s 的 LEFT 先于 v 的 LEFT：
1. s 插入 status，status = {s}
2. v 插入 status，status = {s, v}（v 在 s 上面）
3. 但此时错过了检测 s 和 v 交点的时机！
```

------

## 四、总结对比

| 特性             | EventComp             | SegmentComparator      |
| ---------------- | --------------------- | ---------------------- |
| **容器**         | priority_queue        | set                    |
| **排序依据**     | 事件处理顺序          | 当前扫描线位置的 y 值  |
| **是否动态**     | 否（事件固定）        | 是（随 currentX 变化） |
| **垂直线段处理** | LEFT 优先，RIGHT 段后 | 总是排在非垂直下面     |
| **核心逻辑**     | x → type → y          | y(currentX)            |
| **目的**         | 决定事件处理顺序      | 维护线段上下关系       |

------

这两个比较器是扫描线算法正确性的核心，它们的精心设计确保了：

1. 事件按正确顺序处理
2. 线段在状态结构中的顺序正确反映几何关系
3. 所有交点都能被检测到