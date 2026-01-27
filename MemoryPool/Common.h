#pragma once
#include "Vert_Segment.h"
#include <cstddef>
#include <thread>
namespace ysk
{

// Test parameters - use inline to avoid multiple definition errors
static size_t TOTAL_ITEM = 20;
inline size_t DELTA_THETA = 10; //一定能被720整除

inline size_t TOTAL_VERT = TOTAL_ITEM * TOTAL_ITEM * 720 / DELTA_THETA * 80;
inline size_t TOTAL_SEGMENT = TOTAL_VERT / 2; //两个Vert对应一个Segment

inline size_t TOTAL_VERT_SIZE = TOTAL_VERT * sizeof(Vert);
inline size_t TOTAL_SEGMENT_SIZE = TOTAL_SEGMENT * sizeof(Segment);

inline size_t NUM_THREADS = std::thread::hardware_concurrency();

inline size_t VERT_PER_THREAD = TOTAL_VERT / NUM_THREADS;
inline size_t SEGMENT_PER_THREAD = TOTAL_SEGMENT / NUM_THREADS;

inline size_t VERT_NEED_SIZE_PER_THREAD = TOTAL_VERT_SIZE / NUM_THREADS;
inline size_t SEGMENT_NEED_SIZE_PER_THREAD = TOTAL_SEGMENT_SIZE / NUM_THREADS;


// 内存块对齐大小
constexpr size_t ALIGNMENT = sizeof(Segment);
constexpr size_t MAX_BYTES = 256 * 1024;
constexpr size_t FREE_LIST_SIZE = 2;

#ifdef _MSC_VER
    #define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define FORCE_INLINE inline
#endif

//暂时未使用
class SizeClass
{
public:
    //计算块大小对应的对齐字节数方式  1
    static FORCE_INLINE size_t roundUp(size_t bytes)
    {
        return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }

    //计算块大小对应的对齐字节数方式  2
    static FORCE_INLINE size_t ByteSize(size_t index)
    {
        return (index + 1) * ALIGNMENT;
    }

    //该函数计算给定字节数对应的空闲列表索引
    static FORCE_INLINE size_t getIndex(size_t bytes)
    {
        bytes = std::max(bytes, ALIGNMENT);
        return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
    }

};

}
