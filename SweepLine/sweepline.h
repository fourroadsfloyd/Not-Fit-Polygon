#ifndef SWEEPLINE_H
#define SWEEPLINE_H

#include <QDebug>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <set>
#include <QPointF>


inline pointKey keyOf(const Point& p)
{
    uint32_t ux = static_cast<uint32_t>(static_cast<int32_t>(p.x));
    uint32_t uy = static_cast<uint32_t>(static_cast<int32_t>(p.y));
    return (static_cast<uint64_t>(ux) << 32) | uy;
}

// ---------- 事件 ----------
enum EventType {
    LEFT,
    RIGHT
};

class Event {
public:

    Vert* v;  //25-10-1

    Segment* seg;      // 用于端点事件

    EventType type;

    // 端点事件构造函数
    Event(Vert* v_, Segment* seg_, EventType type_)
        : v(v_), seg(seg_), type(type_) {}

};

class EventComp{
public:
    bool operator()(const Event& A, const Event& B) const
    {
        if (A.v->pt.x != B.v->pt.x)
            return A.v->pt.x > B.v->pt.x; // x小的优先弹出

        if(!A.seg->existlk && !B.seg->existlk && A.type == 0 && B.type == 0)
        {
            return A.v->pt.y > B.v->pt.y;
        }

        if(!A.seg->existlk && A.type == 0)
        {
            return false;                  //A先弹出
        }

        if(!B.seg->existlk && B.type == 0)
        {
            return true;                   //B先弹出
        }

        if(!A.seg->existlk && !B.seg->existlk && A.type == 1 && B.type == 1)
        {
            return A.v->pt.y > B.v->pt.y;  //都是垂直且都是右事件，y小的先弹出
        }

        if(!A.seg->existlk && A.type == 1)
        {
            return true;                   //B先弹出
        }

        if(!B.seg->existlk && B.type == 1)
        {
            return false;                  //A先弹出
        }

        //扫描线左边的线段只有与当前扫描线位置的垂直线段存在交点，非垂直不可能存在交点
        //比较type前，已经先让垂直线段进入了set，这样避免了漏判交点。
        //当前扫描线位置不存在垂直线段时，则先让扫描线左侧的线段先出，即right事件先出。
        if (A.type != B.type)
            return A.type < B.type;     // left < right

        return A.v->pt.y > B.v->pt.y;
    }
};

// ---------- 比较器（用于set排序） ----------
/*
 * 在std::set<Key,Compare>里,比较器cmp只干一件事:回答“第一个参数是否必须排在第二个参数前面?”
 */
class SegmentComparator {
public:

    int64* currentX; // 需要访问当前扫描线位置

    SegmentComparator(int64* x) : currentX(x) {}

    inline bool operator()(Segment* a, Segment* b) const {
        // 计算两条线段在当前扫描线位置的y值
        double ya = getYAtX(*a,*currentX);
        double yb = getYAtX(*b,*currentX);

        if(!a->existlk && !b->existlk)
        {
            return ya < yb;
        }

        if(std::fabs(ya - yb) < 3)
        {
            if(!a->existlk)             //将垂直线段放在下面
            {
                return false;
            }

            if(!b->existlk)
            {
                return true;
            }

            ya = getYAtX(*a,*currentX + 1);
            yb = getYAtX(*b,*currentX + 1);
        }

        return ya < yb;
    }

    inline double getYAtX(const Segment& s, int64 x) const
    {
        if (s.existlk)
        {
            return (double)(s.lk * x + s.lb);
        }

        return (double)(std::min(s.a->pt.y, s.b->pt.y));
    }
};

using EventQue = std::priority_queue<Event,std::vector<Event>,EventComp>;

// ---------- 扫描线 ----------
class SweepLine {
public:

    //SweepLine();
    SweepLine() : status(comp){}

    ~SweepLine();

    //存储原始线段
    std::vector<Segment*> segments;

    std::vector<Vert*> verts;

    std::unordered_map<pointKey, int> vertId;

    void run(std::vector<Segment *> &input_);

private:

    inline bool vertexDeduplicate(Vert* vert);

    inline void handleLeftEvent(const Event& e);

    inline void handleRightEvent(const Event& e);

    inline void checkNeighbours(std::set<Segment*>::iterator it, Segment* seg);

    inline void checkNeighboursBeforeRemoval(std::set<Segment*>::iterator it);

    //将线段的端点以事件形式插入优先级队列
    inline void insertPriorityQueue(Segment* s, bool isleft);

    inline Vert *insertVerts(Vert* ip);

    inline void modifyVertInSeg(Segment* s, Vert* ip, bool aorb);    //a:true b:false

    inline bool processIntersect(Segment *s1, Segment *s2); //插入s2时，处理与其左右邻居是否有交点

    inline bool processIntersectPrevNext(Segment *s1, Segment *s2); //用于从set中取出右端点事件前，比较右线段所在位置处，邻居是否有交点

    inline Vert *intersect(Segment& s1, Segment& s2);

    //当前扫描线位置
    int64 currentX = LLONG_MIN;

    //set中比较
    SegmentComparator comp{&currentX};

    // 创建事件队列
    EventQue Q;

    std::set<Segment*, SegmentComparator> status;   //线段在set中的顺序

    //======测试打印控制======
    bool handleLeftEvent_flag = false;
    bool handleRightEvent_flag = false;
    bool insertPriorityQueue_flag = false;
    bool checkNeighbours_flag = false;
    bool checkNeighboursBeforeRemoval_flag = false;
    bool processIntersect_flag = false;
    bool processIntersectPrevNext_falg = false;

};


#endif // SWEEPLINE_H
