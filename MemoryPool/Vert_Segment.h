#pragma once
#include <vector>


class ThreadCache;

// 前向声明 Segment
class Segment;
class Point;

class Vert {
public:
    Vert() = default;

    Vert(const Point& pt_)  // ✅ 改为 const 引用
    {
        pt = pt_;
    }

    void init(const Point& pt_)  // ✅ 改为 const 引用，接受左值和右值
    {
        pt = pt_;
        vec_start_point_segment.clear();  // ✅ 清空旧的关联
        vec_end_point_segment.clear();
        vec_start_point_segment.reserve(8);
        vec_end_point_segment.reserve(8);
    }

    void reset()
    {
        vec_start_point_segment.clear();
        vec_end_point_segment.clear();
    }

    void* operator new(size_t size);
    void operator delete(void* ptr, size_t size);

    Point pt;

    std::vector<Segment*> vec_start_point_segment;    //存储线段起点为pt的线段
    std::vector<Segment*> vec_end_point_segment;      //存储线段末点为pt的线段
};

// ---------- 线段 ----------                                                     线段的格式需要这样定义吗？
class Segment {
public:
    Segment() = default;

    void init(Vert* a_, Vert* b_)
    {
        a = a_;
        b = b_;

        // ✅ 添加关联（init 总是在 reset 后调用，所以不需要检查重复）
        a->vec_start_point_segment.push_back(this);
        b->vec_end_point_segment.push_back(this);

        // 计算几何属性
        if(a->pt.x == b->pt.x)
        {
            existlk = false;
        }
        else
        {
            lk =(double)(a->pt.y - b->pt.y) / (double)(a->pt.x - b->pt.x);
            lb = a->pt.y - lk * a->pt.x;
        }

        Point p = b->pt - a->pt;

        this->angle = atan2(p.y,p.x);

        if(this->angle < 0)
        {
            this->angle += 2 * M_PI;
        }

        if(std::fabs(angle - 2*M_PI) < 1e-6)
        {
            this->angle = 0;
        }
    }

    Segment(Vert* a_, Vert* b_)
    {
        a = a_;
        b = b_;

        a->vec_start_point_segment.push_back(this); //this的起点是a.pt
        b->vec_end_point_segment.push_back(this);   //this的末点是b.pt

        init();
    }

    void init()
    {
        if(a->pt.x == b->pt.x)
        {
            existlk = false;
        }
        else
        {
            lk =(double)(a->pt.y - b->pt.y) / (double)(a->pt.x - b->pt.x);

            lb = a->pt.y - lk * a->pt.x;
        }

        Point p = b->pt - a->pt;

        this->angle = atan2(p.y,p.x);

        if(this->angle < 0)
        {
            this->angle += 2 * M_PI;
        }

        if(std::fabs(angle - 2*M_PI) < 1e-6)
        {
            this->angle = 0;
        }
    }

    void reset()
    {
        // ✅ 必须清理 Vert 的 vector 中的指针
        // 因为 Vert 可能还在使用，不能留下悬挂指针
        if (a) {
            auto& vec = a->vec_start_point_segment;
            vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
        }
        if (b) {
            auto& vec = b->vec_end_point_segment;
            vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
        }
        
        // 重置指针
        a = nullptr;
        b = nullptr;
        prev = nullptr;
        next = nullptr;
        
        // 重置状态
        existlk = true;
        isvalid = true;
        lk = 0.0;
        lb = 0.0;
        angle = 0.0;
    }

    void* operator new(size_t size);
    void operator delete(void* ptr, size_t size);

    bool existlk = true;

    double lk; //斜率
    double lb; //b   y=kx+b

    double angle;   //相对于正向x轴

    Vert* a, *b;

    Segment* prev = nullptr;  // 记录指向当前线段的线段
    Segment* next = nullptr;  // 记录当前线段指向的线段

    bool isvalid = true;
};
