#include "sweepline.h"

SweepLine::~SweepLine()
{
    for(Segment* s : this->segments)
    {
        if(g_use_obj_pool)
            ObjectPool<Segment>::getInstance().release(s);
        else
            delete s;
    }

    this->segments.clear();

    for(Vert* v : this->verts)
    {
        if(g_use_obj_pool)
            ObjectPool<Vert>::getInstance().release(v);
        else
            delete v;
    }

    this->verts.clear();
}

void SweepLine::run(std::vector<Segment *> &input_)
{
    segments = input_;
    vertId.clear();
    verts.clear();
    status.clear();

    //qDebug() << "segments.size = "<<segments.size();

    QElapsedTimer time2;

    time2.start();

    //顶点去重
    for(Segment* seg : input_)
    {
        Vert* vert = seg->a;
        bool flag = this->vertexDeduplicate(seg->a);
        if(flag)    //ture就需要删除seg->a
        {
            if(g_use_obj_pool)
                ObjectPool<Vert>::getInstance().release(vert);
            else
            {
                delete vert;
                vert = nullptr;
            }
        }

        vert = seg->b;
        flag = this->vertexDeduplicate(seg->b);
        if(flag)
        {
            if(g_use_obj_pool)
                ObjectPool<Vert>::getInstance().release(vert);
            else
            {
                delete vert;
                vert = nullptr;
            }
        }
    }


    for (Segment* s : input_)
    {
        insertPriorityQueue(s,true);    //只插入左侧端点
    }

    //优先队列创建结束
    while (!Q.empty())  //事件队列，不为空时一直循环
    {
        EventQue EQ = Q;

        Event e = Q.top();

        Q.pop();

        switch (e.type)
        {
        case LEFT:
            handleLeftEvent(e);
            insertPriorityQueue(e.seg,false);
            break;
        case RIGHT:
            handleRightEvent(e);
            break;
        }
    }
}

//返回true就需要删除vert
inline bool SweepLine::vertexDeduplicate(Vert* vert)
{
    pointKey k = keyOf(vert->pt);
    auto it = vertId.find(k);

    if (it != vertId.end())
    {
        //去重操作
        for(Segment* seg : vert->vec_start_point_segment)
        {
            seg->a = verts[it->second];
        }

        for(Segment* seg : vert->vec_end_point_segment)
        {
            seg->b = verts[it->second];
        }

        verts[it->second]->vec_start_point_segment.insert(verts[it->second]->vec_start_point_segment.end(),
                                                          vert->vec_start_point_segment.begin(),
                                                          vert->vec_start_point_segment.end());

        verts[it->second]->vec_end_point_segment.insert(verts[it->second]->vec_end_point_segment.end(),
                                                        vert->vec_end_point_segment.begin(),
                                                        vert->vec_end_point_segment.end());

        return true;
    }

    int id = verts.size();
    vertId.insert({k,id});
    verts.push_back(vert);

    return false;
}

inline void SweepLine::handleLeftEvent(const Event &e)
{
    // 更新比较器的当前扫描线位置
    currentX = e.v->pt.x;

    auto result = status.insert(e.seg);

    //qDebug() << "result = " << *result.first;

    if (result.second)
    {
        if(handleLeftEvent_flag)
        {
            qDebug() << "====insert success ===="
                     << "currentX = " << currentX << " "
                     << "seg = " << e.seg << " "
                     << "seg.existlk = " << e.seg->existlk
                     << e.seg->a->pt.x << " " << e.seg->a->pt.y << " "
                     << e.seg->b->pt.x << " " << e.seg->b->pt.y;

            //插入成功后打印set中的线段顺序
            qDebug() << "insert one seg status order======";
            for (auto it = status.begin(); it != status.end(); ++it)
            {
                Segment* seg = *it;          // 解引用得到 Segment*

                qDebug() << "currentX = " << currentX << " "
                         << "seg = " << seg << " "
                         << "seg.existlk" << seg->existlk
                         << seg->a->pt.x << " " << seg->a->pt.y << " "
                         << seg->b->pt.x << " " << seg->b->pt.y;
            }
        }

        checkNeighbours(result.first, e.seg);
    }
    else
    {
        if(handleLeftEvent_flag)
        {
            qDebug() << "====insert false===="
                     << "currentX = " << currentX << " "
                     << "seg = " << e.seg << " "
                     << e.seg->a->pt.x << " " << e.seg->a->pt.y << " "
                     << e.seg->b->pt.x << " " << e.seg->b->pt.y;
        }
    }
}

inline void SweepLine::handleRightEvent(const Event &e)
{
    auto it = status.find(e.seg);

    if (it != status.end())
    {

        checkNeighboursBeforeRemoval(it);

        status.erase(it);

        if(handleRightEvent_flag)
        {
            qDebug() << "====remove seg ===="
                     << "currentX = " << currentX << " "
                     << "seg = " << e.seg << " "
                     << e.seg->a->pt.x << " " << e.seg->a->pt.y << " "
                     << e.seg->b->pt.x << " " << e.seg->b->pt.y;

            qDebug() << "remove one seg: status order======";
            for (auto it = status.begin(); it != status.end(); ++it)
            {
                Segment* seg = *it;          // 解引用得到 Segment*
                // 下面随便用
                qDebug() << "currentX = " << currentX << " "
                         << "seg = " << seg << " "
                         << seg->a->pt.x << " " << seg->a->pt.y << " "
                         << seg->b->pt.x << " " << seg->b->pt.y;
            }
        }
    }
    else
    {
        if(handleRightEvent_flag)
        {
            qDebug() << "MISSING seg in status @ RIGHT event"
                     << "currentX = " << currentX << " "
                     << " seg=" << e.seg << " "
                     << e.seg->a->pt.x << " " << e.seg->a->pt.y << " "
                     << e.seg->b->pt.x << " " << e.seg->b->pt.y;
        }
    }
}

inline void SweepLine::checkNeighbours(std::set<Segment*>::iterator it, Segment* seg)
{
    // 检查上邻居
    if (it != status.begin())
    {
        auto prev = std::prev(it);

        bool flag = processIntersect(*prev, seg);

        if(flag == false)
        {
            if(checkNeighbours_flag)
            {
                qDebug() << "SweepLine::checkNeighbours : prev don't have intersection";
                qDebug() << "prev seg ===\n"
                         << (*prev)->a->pt.x << " " << (*prev)->a->pt.y << " "
                         << (*prev)->b->pt.x << " " << (*prev)->b->pt.y << "\n"
                         << "now  seg ===\n"
                         << seg->a->pt.x << " " << seg->a->pt.y << " "
                         << seg->b->pt.x << " " << seg->b->pt.y << "\n";
            }
        }
    }

    // 检查下邻居
    auto next = std::next(it);

    if (next != status.end())
    {
        bool flag = processIntersect(*next, seg);
        if(flag == false)
        {
            if(checkNeighbours_flag)
            {
                qDebug() << "SweepLine::checkNeighbours : next don't have intersection";
                qDebug() << "next seg ===\n"
                         << (*next)->a->pt.x << " " << (*next)->a->pt.y << " "
                         << (*next)->b->pt.x << " " << (*next)->b->pt.y << "\n"
                         << "now  seg ===\n"
                         << seg->a->pt.x << " " << seg->a->pt.y << " "
                         << seg->b->pt.x << " " << seg->b->pt.y << "\n";
            }
        }
    }
}

inline void SweepLine::checkNeighboursBeforeRemoval(std::set<Segment*>::iterator it)
{
    // 检查移除后可能新相邻的线段
    if (it != status.begin())
    {
        auto prev = std::prev(it);
        auto next = std::next(it);
        if (next != status.end())
        {
            bool flag = processIntersectPrevNext(*prev, *next);
            if(flag == false)
            {
                if(checkNeighboursBeforeRemoval_flag)
                {
                    qDebug() << "SweepLine::checkNeighboursBeforeRemoval : prev and next don't have intersection";
                    qDebug() << "prev seg ===\n"
                             << (*prev)->a->pt.x << " " << (*prev)->a->pt.y << " "
                             << (*prev)->b->pt.x << " " << (*prev)->b->pt.y << "\n"
                             << "next seg ===\n"
                             << (*next)->a->pt.x << " " << (*next)->a->pt.y << " "
                             << (*next)->b->pt.x << " " << (*next)->b->pt.y << "\n";
                }
            }
        }
    }
}

inline void SweepLine::insertPriorityQueue(Segment *s,bool isleft)
{   
    if(s == nullptr)
    {
        qDebug() << "SweepLine::insertPriorityQueue: s is nullptr";
        return;
    }

    if (s->a->pt.x < s->b->pt.x)
    {
        if(isleft)
        {
            Q.push(Event(s->a, s, LEFT));
            if(insertPriorityQueue_flag)
            {
                qDebug() << "========insert a new Event========\n"
                         << "type = " << EventType::LEFT << " "
                         << "seg = " << s << " "
                         << s->a->pt.x << " " << s->a->pt.y;
            }
        }
        else
        {
            Q.push(Event(s->b, s, RIGHT));
            if(insertPriorityQueue_flag)
            {
                qDebug() << "========insert a new Event========\n"
                         << "type = " << EventType::RIGHT << " "
                         << "seg = " << s << " "
                         << s->b->pt.x << " " << s->b->pt.y;
            }
        }
    }
    else if(s->a->pt.x == s->b->pt.x)
    {
        if(s->a->pt.y < s->b->pt.y)
        {
            if(isleft)
            {
                Q.push(Event(s->a, s, LEFT));
                if(insertPriorityQueue_flag)
                {
                    qDebug() << "========insert a new Event========\n"
                             << "type = " << EventType::LEFT << " "
                             << "seg = " << s << " "
                             << s->a->pt.x << "," << s->a->pt.y;
                }
            }
            else
            {
                Q.push(Event(s->b, s, RIGHT));
                if(insertPriorityQueue_flag)
                {
                    qDebug() << "========insert a new Event========\n"
                             << "type = " << EventType::RIGHT << " "
                             << "seg = " << s << " "
                             << s->b->pt.x << "," << s->b->pt.y;
                }
            }
        }
        else
        {
            if(isleft)
            {
                Q.push(Event(s->b, s, LEFT));
                if(insertPriorityQueue_flag)
                {
                    qDebug() << "========insert a new Event========\n"
                             << "type = " << EventType::LEFT << " "
                             << "seg = " << s << " "
                             << s->b->pt.x << "," << s->b->pt.y;
                }
            }
            else
            {
                Q.push(Event(s->a, s, RIGHT));
                if(insertPriorityQueue_flag)
                {
                    qDebug() << "========insert a new Event========\n"
                             << "type = " << EventType::RIGHT << " "
                             << "seg = " << s << " "
                             << s->a->pt.x << "," << s->a->pt.y;
                }
            }
        }
    }
    else
    {
        if(isleft)
        {
            Q.push(Event(s->b, s, LEFT));
            if(insertPriorityQueue_flag)
            {
                qDebug() << "========insert a new Event========\n"
                         << "type = " << EventType::LEFT << " "
                         << "seg = " << s << " "
                         << s->b->pt.x << "," << s->b->pt.y;
            }
        }
        else
        {
            Q.push(Event(s->a, s, RIGHT));
            if(insertPriorityQueue_flag)
            {
                qDebug() << "========insert a new Event========\n"
                         << "type = " << EventType::RIGHT << " "
                         << "seg = " << s << " "
                         << s->a->pt.x << "," << s->a->pt.y;
            }
        }
    }
}

inline Vert* SweepLine::insertVerts(Vert *ip)
{
    pointKey key = keyOf(ip->pt);

    auto it = vertId.find(key);

    if(it == vertId.end())  //不包含就把ip加入
    {
        int id = verts.size();
        vertId.insert({key,id});
        verts.push_back(ip);

        return ip;
    }

    if(ip != verts[it->second])
    {
        if(g_use_obj_pool)
            ObjectPool<Vert>::getInstance().release(ip);
        else
            delete ip;
    }

    return verts[it->second];
}

auto removeInOut = [](std::vector<Segment*>& vec, Segment* s) {
    vec.erase(std::remove(vec.begin(), vec.end(), s), vec.end());
};

inline void SweepLine::modifyVertInSeg(Segment *s, Vert *ip, bool aorb)
{
    if(aorb)
    {
        removeInOut(s->a->vec_start_point_segment, s);

        s->a = ip;

        s->a->vec_start_point_segment.push_back(s);

        //如果使用intersect中的changeInter_V_SS时，必须使用init
        //s->init();
    }
    else
    {
        removeInOut(s->b->vec_end_point_segment, s);   // 重点：把旧终点登记清掉

        s->b = ip;

        s->b->vec_end_point_segment.push_back(s);

        //如果使用intersect中的changeInter_V_SS时，必须使用init
        //s->init();
    }
}


bool SweepLine::processIntersect(Segment* s1, Segment* s2)
{
    Vert* ip = intersect(*s1, *s2);

    if(ip == nullptr)
    {
        return false;
    }

    if(ip->pt.x < currentX)
    {
        qDebug() << "error::SweepLine::processIntersect x in left, currentX = "<< currentX << "ip.x = " << ip->pt.x;

        //delete ip;

        return false;
    }

    ip = insertVerts(ip);   //确保ip在Verts中

    if(ip == nullptr)
    {
        qDebug() << "error::SweepLine::insertVerts ret val is nullptr";
        return false;
    }

    if(s1->a != ip && s1->b != ip)    //说明交点在s1内部
    {
        if((!s1->existlk && s1->a->pt.y < s1->b->pt.y) ||
            (s1->a->pt.x < s1->b->pt.x))   //  a<x<b a是left
        {
            Segment* seg = nullptr;
            if(g_use_obj_pool)
                seg = ObjectPool<Segment>::getInstance().acquire(ip, s1->b);
            else
                seg = new Segment(ip,s1->b); //a是左端点

            if(seg->existlk == false && s1->existlk)
            {
                seg->existlk = s1->existlk;
                seg->lk = s1->lk;
                seg->lb = s1->lb;
                seg->angle = s1->angle;

                if(processIntersect_flag)
                {
                    qDebug() << "processIntersect lk change 1";
                }
            }

            segments.push_back(seg);

            insertPriorityQueue(seg,true);

            modifyVertInSeg(s1,ip,false);

            insertPriorityQueue(s1,false); //优先队列中会出现两个s1的右端点
        }
        else    //b是左端点
        {
            Segment* seg = nullptr;
            if(g_use_obj_pool)
                seg = ObjectPool<Segment>::getInstance().acquire(s1->a,ip);
            else
                seg = new Segment(s1->a,ip);

            if(seg->existlk == false && s1->existlk)
            {
                seg->existlk = s1->existlk;
                seg->lk = s1->lk;
                seg->lb = s1->lb;
                seg->angle = s1->angle;

                if(processIntersect_flag)
                {
                    qDebug() << "processIntersect lk change 2";
                }
            }

            segments.push_back(seg);

            insertPriorityQueue(seg,true);

            modifyVertInSeg(s1,ip,true);

            insertPriorityQueue(s1,false);
        }
    }

    if(s2->a != ip && s2->b != ip)    //说明交点在s2内部
    {
        if((!s2->existlk && s2->a->pt.y < s2->b->pt.y) ||
            (s2->a->pt.x < s2->b->pt.x))
        {
            Segment* seg = nullptr;
            if(g_use_obj_pool)
                seg = ObjectPool<Segment>::getInstance().acquire(ip,s2->b);
            else
                seg = new Segment(ip,s2->b); //a是左端点

            if(seg->existlk == false && s2->existlk)
            {
                seg->existlk = s2->existlk;
                seg->lk = s2->lk;
                seg->lb = s2->lb;
                seg->angle = s2->angle;

                if(processIntersect_flag)
                {
                    qDebug() << "processIntersect lk change 3";
                }
            }

            segments.push_back(seg);

            insertPriorityQueue(seg,true);

            modifyVertInSeg(s2,ip,false);
        }
        else    //b是左端点
        {
            Segment* seg = nullptr;
            if(g_use_obj_pool)
                seg = ObjectPool<Segment>::getInstance().acquire(s2->a,ip);
            else
                seg = new Segment(s2->a,ip);

            if(seg->existlk == false && s2->existlk)
            {
                seg->existlk = s2->existlk;
                seg->lk = s2->lk;
                seg->lb = s2->lb;
                seg->angle = s2->angle;

                if(processIntersect_flag)
                {
                    qDebug() << "processIntersect lk change 4";
                }
            }


            segments.push_back(seg);

            insertPriorityQueue(seg,true);

            modifyVertInSeg(s2,ip,true);
        }
    }

    return true;
}


inline bool SweepLine::processIntersectPrevNext(Segment* s1, Segment* s2)
{

    Vert* ip = intersect(*s1, *s2);

    if(ip == 0)
    {
        return false;
    }

    if(ip->pt.x < currentX)
    {
        qDebug() << "error::SweepLine::processIntersectPrevNext x in left, currentX = "<< currentX << "ip.x = " << ip->pt.x;

        return false;
    }

    ip = insertVerts(ip);   //确保ip在Verts中

    if(s1->a != ip && s1->b != ip)    //说明交点在s1内部
    {
        if((!s1->existlk && s1->a->pt.y < s1->b->pt.y) ||
            (s1->a->pt.x < s1->b->pt.x))
        {
            Segment* seg = nullptr;
            if(g_use_obj_pool)
                seg = ObjectPool<Segment>::getInstance().acquire(ip,s1->b);
            else
                seg = new Segment(ip,s1->b); //a是左端点

            if(seg->existlk == false && s1->existlk)
            {
                seg->existlk = s1->existlk;
                seg->lk = s1->lk;
                seg->lb = s1->lb;
                seg->angle = s1->angle;

                if(processIntersectPrevNext_falg)
                {
                    qDebug() << "processIntersectPrevNext lk change 1";
                }
            }

            segments.push_back(seg);

            insertPriorityQueue(seg,true);

            modifyVertInSeg(s1,ip,false);

            insertPriorityQueue(s1,false); //优先队列中会出现两个s1的右端点
        }
        else    //b是左端点
        {
            Segment* seg = nullptr;
            if(g_use_obj_pool)
                seg = ObjectPool<Segment>::getInstance().acquire(s1->a,ip);
            else
                seg = new Segment(s1->a,ip);

            if(seg->existlk == false && s1->existlk)
            {
                seg->existlk = s1->existlk;
                seg->lk = s1->lk;
                seg->lb = s1->lb;
                seg->angle = s1->angle;

                if(processIntersectPrevNext_falg)
                {
                    qDebug() << "processIntersectPrevNext lk change 1";
                }
            }

            segments.push_back(seg);

            insertPriorityQueue(seg,true);

            modifyVertInSeg(s1,ip,true);

            insertPriorityQueue(s1,false);
        }
    }

    if(s2->a != ip && s2->b != ip)    //说明交点在s2内部
    {
        if((!s2->existlk && s2->a->pt.y < s2->b->pt.y) ||
            (s2->a->pt.x < s2->b->pt.x))
        {
            Segment* seg = nullptr;
            if(g_use_obj_pool)
                seg = ObjectPool<Segment>::getInstance().acquire(ip,s2->b);
            else
                seg = new Segment(ip,s2->b); //a是左端点

            if(seg->existlk == false && s2->existlk)
            {
                seg->existlk = s2->existlk;
                seg->lk = s2->lk;
                seg->lb = s2->lb;
                seg->angle = s2->angle;

                if(processIntersectPrevNext_falg)
                {
                    qDebug() << "processIntersectPrevNext lk change 1";
                }
            }

            segments.push_back(seg);

            insertPriorityQueue(seg,true);

            modifyVertInSeg(s2,ip,false);

            insertPriorityQueue(s2,false); //优先队列中会出现两个s2的右端点
        }
        else    //b是左端点
        {
            Segment* seg = nullptr;
            if(g_use_obj_pool)
                seg = ObjectPool<Segment>::getInstance().acquire(s2->a,ip);
            else
                seg = new Segment(s2->a,ip);

            if(seg->existlk == false && s2->existlk)
            {
                seg->existlk = s2->existlk;
                seg->lk = s2->lk;
                seg->lb = s2->lb;
                seg->angle = s2->angle;

                if(processIntersectPrevNext_falg)
                {
                    qDebug() << "processIntersectPrevNext lk change 1";
                }
            }

            segments.push_back(seg);

            insertPriorityQueue(seg,true);

            modifyVertInSeg(s2,ip,true);

            insertPriorityQueue(s2,false);
        }
    }

    return true;
}

inline auto cross = [](const Point& a,const Point& b) ->int64{
    return a.x * b.y - a.y * b.x;
};

inline auto changeInter_V_S_S = [](Segment& seg1, Segment& seg2, Vert* v) -> Vert*
{
    if(v == nullptr)
        return v;

    bool seg1_degenerate = seg1.existlk && (v->pt.x == seg1.a->pt.x || v->pt.x == seg1.b->pt.x);
    bool seg2_degenerate = seg2.existlk && (v->pt.x == seg2.a->pt.x || v->pt.x == seg2.b->pt.x);

    if(!seg1_degenerate && !seg2_degenerate) return v;

    Point candidates[6] = {
        Point(v->pt.x+1, v->pt.y),     // 右
        Point(v->pt.x+1, v->pt.y+1),   // 右上
        Point(v->pt.x+1, v->pt.y-1),   // 右下
        Point(v->pt.x-1, v->pt.y),     // 左
        Point(v->pt.x-1, v->pt.y+1),   // 左上
        Point(v->pt.x-1, v->pt.y-1)    // 左下
    };

    Point best_candidate = v->pt;
    bool found_valid = false;

    for(const Point& p : candidates)
    {
        // 检查这个候选点是否会导致任何线段变垂直
        bool p_seg1_degenerate = false;
        bool p_seg2_degenerate = false;

        if(seg1.existlk)
        {
            p_seg1_degenerate = (p.x == seg1.a->pt.x || p.x == seg1.b->pt.x);
        }

        if(seg2.existlk)
        {
            p_seg2_degenerate = (p.x == seg2.a->pt.x || p.x == seg2.b->pt.x);
        }

        if(p_seg1_degenerate || p_seg2_degenerate)
        {
            continue;
        }
        else
        {
            best_candidate = p;
            found_valid = true;
            break;
        }
    }

    if(found_valid && best_candidate != v->pt)
    {
        v->pt = best_candidate;
    }

    return v;
};

inline Vert* SweepLine::intersect(Segment &s1, Segment &s2)
{
    // 快速排斥实验
    if (std::max(s1.a->pt.x, s1.b->pt.x) < std::min(s2.a->pt.x, s2.b->pt.x) ||
        std::max(s1.a->pt.y, s1.b->pt.y) < std::min(s2.a->pt.y, s2.b->pt.y) ||
        std::max(s2.a->pt.x, s2.b->pt.x) < std::min(s1.a->pt.x, s1.b->pt.x) ||
        std::max(s2.a->pt.y, s2.b->pt.y) < std::min(s1.a->pt.y, s1.b->pt.y))
    {
      //  qDebug() << "1";
        return nullptr;
    }

    if (s1.a->pt == s2.a->pt ||
        s1.a->pt == s2.b->pt ||
        s1.b->pt == s2.a->pt ||
        s1.b->pt == s2.b->pt )
    {
      //  qDebug() << "2";
        return nullptr;
    }

    // 计算向量
    Point vec_AB = s1.b->pt - s1.a->pt;
    Point vec_AC = s2.a->pt - s1.a->pt;
    Point vec_AD = s2.b->pt - s1.a->pt;
    Point vec_CD = s2.b->pt - s2.a->pt;
    Point vec_CA = s1.a->pt - s2.a->pt;
    Point vec_CB = s1.b->pt - s2.a->pt;

    // 计算叉积
    int64 cross1 = cross(vec_AB, vec_AC);
    int64 cross2 = cross(vec_AB, vec_AD);
    int64 cross3 = cross(vec_CD, vec_CA);
    int64 cross4 = cross(vec_CD, vec_CB);

    // 检查共线情况
    if (cross1 == 0 && cross2 == 0) {
        // 线段共线，检查是否有重叠
        // 将线段投影到X轴（如果线段是垂直的，则投影到Y轴）
        bool useX = (std::abs(s1.a->pt.x - s1.b->pt.x)) > (std::abs(s1.a->pt.y - s1.b->pt.y));
        int64 a1 = useX ? s1.a->pt.x : s1.a->pt.y;
        int64 b1 = useX ? s1.b->pt.x : s1.b->pt.y;
        int64 c1 = useX ? s2.a->pt.x : s2.a->pt.y;
        int64 d1 = useX ? s2.b->pt.x : s2.b->pt.y;

        // 确保 a1 <= b1, c1 <= d1
        if (a1 > b1) std::swap(a1, b1);
        if (c1 > d1) std::swap(c1, d1);

        int64 L = std::max(a1, c1);
        int64 R = std::min(b1, d1);
        if (L >= R)
        {
            return nullptr; // 无重叠
        }

        return nullptr;
    }

    // 检查端点在线段上的情况
    if (cross1 == 0 ) {
        // s2.a 在 s1 上
       // qDebug() << "3";
        if(my_math_dot(vec_AC,vec_CB) > 0)
        {
           return s2.a;
        }
        else
        {
            return nullptr;
        }
    }
    if (cross2 == 0) {
       // qDebug() << "4";
        // s2.b 在 s1 上
        Point vec_BD = s2.b->pt-s1.b->pt;
        if(my_math_dot(vec_AD,vec_BD) < 0)
        {
            return s2.b;
        }
        else
        {
            return nullptr;
        }

    }
    if (cross3 == 0) {
        // s1.a 在 s2 上
       // qDebug() << "5";
        if(my_math_dot(vec_AC,vec_AD) < 0)
        {
            return s1.a;
        }
        else
        {
            return nullptr;
        }
    }
    if (cross4 == 0) {
        // s1.b 在 s2 上
       // qDebug() << "6";
        Point vec_DB = s1.b->pt-s2.b->pt;
        if(my_math_dot(vec_CB,vec_DB) < 0)
        {
            return s1.b;
        }
        else
        {
            return nullptr;
        }
    }

    // 跨立实验 - 检查线段相交
    if ((double)cross1 * (double)cross2 < 0.0 &&
        (double)cross3 * (double)cross4 < 0.0)          //这里必须转double，否则int64乘法会溢出，且该溢出不会抛出异常
    {
        // 计算交点参数
        double ratio = std::fabs((double)cross1) / std::fabs((double)cross2);

        int64 x = s2.a->pt.x + std::llround(ratio / (1 + ratio) * vec_CD.x);
        int64 y = s2.a->pt.y + std::llround(ratio / (1 + ratio) * vec_CD.y);

        Point p(x,y);

        Vert* ip = nullptr;

        if(g_use_obj_pool)
            ip = ObjectPool<Vert>::getInstance().acquire(p);
        else
            ip = new Vert(p);

        return ip;
    }

    return nullptr;
}


