#include "nfp.h"

Nfp::Nfp(Path &A, Path &B, Paths &result)
{
    this->m_A = &A;
    this->m_B = &B;

    this->m_result = &result;
}

inline Segkey Nfp::getKey(Segment *seg)
{
    Segkey key;

    double angle = seg->angle;

    if(angle >= M_PI)
    {
        angle -= M_PI;
    }

    key = std::llround(angle * 1e6);

    return key;
}

inline bool Nfp::fast_test(Segment &s1, Segment &s2)
{
    // 快速排斥实验
    if (std::max(s1.a->pt.x, s1.b->pt.x) < std::min(s2.a->pt.x, s2.b->pt.x) ||
        std::max(s1.a->pt.y, s1.b->pt.y) < std::min(s2.a->pt.y, s2.b->pt.y) ||
        std::max(s2.a->pt.x, s2.b->pt.x) < std::min(s1.a->pt.x, s1.b->pt.x) ||
        std::max(s2.a->pt.y, s2.b->pt.y) < std::min(s1.a->pt.y, s1.b->pt.y))
    {
        return false;
    }

    return true;
}

/*
 * 返回值： 0 1 2  分别对应删除线段的个数
 */
int Nfp::processOverlap(std::vector<Segment*> & vecSeg, int m,int n)
{
    Segment* s1 = vecSeg[m];
    Segment* s2 = vecSeg[n];

    if(std::abs(s1->angle - s2->angle) > M_PI / 2.0)   
    {
        Point vec = s1->b->pt - s1->a->pt;
        Point vec1 = s2->b->pt - s1->a->pt;

        bool flag1 = false;
        bool flag2 = false;

        if((vec.x * vec1.x + vec.y * vec1.y) > 0)    //说明s2.b在s1.a的右侧
        {
            flag1 = true;
        }

        vec1 = s2->a->pt - s1->b->pt;

        if((vec.x * vec1.x + vec.y * vec1.y) > 0)   //说明s2.a在s1.b的右侧
        {
            flag2 = true;
        }

        if(flag1 && flag2)  //s2.b在s1.a的右侧 ， s2.a在s1.b的右侧
        {
            Point temp = s1->b->pt;

            s1->b->pt = s2->b->pt;
            s1->init();

            s2->b->pt = temp;
            s2->init();
        }
        else if(flag1 && flag2 == false) //s2.b在s1.a的右侧，s2.a在s1.b的左侧
        {
            if(s1->b->pt == s2->a->pt)
            {
                s2->a->pt = s1->a->pt;
                s2->init();

                if(g_use_obj_pool)
                {
                    ObjectPool<Vert>::getInstance().release(s1->a);
                    ObjectPool<Vert>::getInstance().release(s1->b);
                    ObjectPool<Segment>::getInstance().release(s1);
                }
                else
                {
                    delete s1->a;
                    s1->a = nullptr;
                    delete s1->b;
                    s1->b = nullptr;
                    delete s1;
                    s1 = nullptr;
                }

                vecSeg[m] = nullptr;
                return 1;
            }
            else
            {
                Point temp = s1->b->pt;
                s1->b->pt = s2->b->pt;
                s1->init();

                s2->b->pt = temp;
                s2->init();
            }
        }
        else if(flag1 == false && flag2) //s2.b在s1.a的左侧，s2.a在s1.b的右侧
        {
            if(s1->a->pt == s2->b->pt)
            {
                s2->b->pt = s1->b->pt;
                s2->init();

                if(g_use_obj_pool)
                {
                    ObjectPool<Vert>::getInstance().release(s1->a);
                    ObjectPool<Vert>::getInstance().release(s1->b);
                    ObjectPool<Segment>::getInstance().release(s1);
                }
                else
                {
                    delete s1->a;
                    s1->a = nullptr;
                    delete s1->b;
                    s1->b = nullptr;
                    delete s1;
                    s1 = nullptr;
                }

                vecSeg[m] = nullptr;
                return 1;
            }
            else
            {
                Point temp = s1->b->pt;
                s1->b->pt = s2->b->pt;
                s1->init();

                s2->b->pt = temp;
                s2->init();
            }
        }
        else    //s2.b在s1.a的左侧，s2.a在s1.b的左侧
        {
            if(s1->a->pt == s2->b->pt && s1->b->pt == s2->a->pt)
            {
                if(g_use_obj_pool)
                {
                    ObjectPool<Vert>::getInstance().release(s1->a);
                    ObjectPool<Vert>::getInstance().release(s1->b);
                    ObjectPool<Segment>::getInstance().release(s1);
                }
                else
                {
                    delete s1->a;
                    s1->a = nullptr;
                    delete s1->b;
                    s1->b = nullptr;
                    delete s1;
                    s1 = nullptr;
                }

                vecSeg[m] = nullptr;

                if(g_use_obj_pool)
                {
                    ObjectPool<Vert>::getInstance().release(s2->a);
                    ObjectPool<Vert>::getInstance().release(s2->b);
                    ObjectPool<Segment>::getInstance().release(s2);
                }
                else
                {
                    delete s2->a;
                    s2->a = nullptr;
                    delete s2->b;
                    s2->b = nullptr;
                    delete s2;
                    s2 = nullptr;
                }
                vecSeg[n] = nullptr;
                return 2;
            }
            else if(s1->a->pt == s2->b->pt && s1->b->pt != s2->a->pt)
            {
                s2->b->pt = s1->b->pt;
                s2->init();

                if(g_use_obj_pool)
                {
                    ObjectPool<Vert>::getInstance().release(s1->a);
                    ObjectPool<Vert>::getInstance().release(s1->b);
                    ObjectPool<Segment>::getInstance().release(s1);
                }
                else
                {
                    delete s1->a;
                    s1->a = nullptr;
                    delete s1->b;
                    s1->b = nullptr;
                    delete s1;
                    s1 = nullptr;
                }
                vecSeg[m] = nullptr;
                return 1;
            }
            else if(s1->a->pt != s2->b->pt && s1->b->pt == s2->a->pt)
            {
                s2->a->pt = s1->a->pt;
                s2->init();

                if(g_use_obj_pool)
                {
                    ObjectPool<Vert>::getInstance().release(s1->a);
                    ObjectPool<Vert>::getInstance().release(s1->b);
                    ObjectPool<Segment>::getInstance().release(s1);
                }
                else
                {
                    delete s1->a;
                    s1->a = nullptr;
                    delete s1->b;
                    s1->b = nullptr;
                    delete s1;
                    s1 = nullptr;
                }
                vecSeg[m] = nullptr;
                return 1;
            }
            else
            {
                Point temp = s1->a->pt;

                s1->a->pt = s2->a->pt;
                s1->init();

                s2->a->pt = temp;
                s2->init();
            }
        }
    }
    else    //线段同向
    {
        Point vec = s1->b->pt - s1->a->pt;
        Point vec1 = s2->a->pt - s1->a->pt;

        Point start,end;

        if((vec.x * vec1.x + vec.y * vec1.y) >= 0)   //s2.a在s1.a的右侧
        {
            start = s1->a->pt;
        }
        else
        {
            start = s2->a->pt;
        }

        vec1 = s2->b->pt - s1->b->pt;
        if((vec.x * vec1.x + vec.y * vec1.y) >= 0)
        {
            end = s2->b->pt;
        }
        else
        {
            end = s1->b->pt;
        }

        s2->a->pt = start;
        s2->b->pt = end;
        s2->init();

        if(g_use_obj_pool)
        {
            ObjectPool<Vert>::getInstance().release(s1->a);
            ObjectPool<Vert>::getInstance().release(s1->b);
            ObjectPool<Segment>::getInstance().release(s1);
        }
        else
        {
            delete s1->a;
            s1->a = nullptr;
            delete s1->b;
            s1->b = nullptr;
            delete s1;
            s1 = nullptr;
        }
        vecSeg[m] = nullptr;

        return 1;
    }

    return 0;
}

void Nfp::simplifySeg()
{
    for(size_t i=0;i<vec_vec_seg.size();i++)
    {


        std::vector<Segment*>& vecSeg = vec_vec_seg[i];
        if(vecSeg.size() < 2)   //该vector容器中仅有一条线段，不必简化，直接跳过
        {
            //qDebug() << "simplifySeg===vec_vec_seg:" << i <<"===just have one Seg,so continue";
            continue;
        }

        for(int m=vecSeg.size()-1;m>=0;m--)
        {


            //qDebug() << "m order=" << m;

            Segment* seg1 = vecSeg[m];

            if(seg1 == nullptr)
            {
                continue;
            }

            int ret = 0;

            for(int n = m-1;n>=0;n--)
            {
                Segment* seg2 = vecSeg[n];

                if(seg2 == nullptr)
                {
                    continue;
                }

                if(!seg1->existlk && !seg2->existlk)    //seg1和seg2都是垂直于x轴的线段
                {
                    if(seg1->a->pt.x != seg2->a->pt.x)
                    {
                        continue;
                    }
                    else    //两条线段垂直于x轴且x相等，需要进一步判断是否重合
                    {
                        if(std::max(seg1->a->pt.y,seg1->b->pt.y) < std::min(seg2->a->pt.y,seg2->b->pt.y))   //线段无重叠
                        {
                            continue;
                        }
                        else if(std::min(seg1->a->pt.y,seg1->b->pt.y) > std::max(seg2->a->pt.y,seg2->b->pt.y))
                        {
                            continue;
                        }
                        else
                        {
                            ret = processOverlap(vecSeg,m,n);

                            if(ret)
                            {
                                break;
                            }
                        }
                    }
                }
                else if(seg1->existlk && seg2->existlk) //两条线段都不是垂直线段
                {
                    if(seg1->lb != seg2->lb)    //平行，否则共线
                        continue;

                    bool flag = fast_test(*seg1,*seg2);
                    if(!flag)
                    {
                        continue;
                    }

                    ret = processOverlap(vecSeg,m,n);
                    if(ret)
                    {
                        break;
                    }
                }
                else
                {
                    continue;
                }
            }
        }
    }
}

inline auto cal_vec_norm = [](Point vec) -> Point{  //右旋90度的法向量
    int64 temp = vec.x;
    vec.x = vec.y;
    vec.y = -temp;
    return vec;
};

inline auto cal_line_len = [](const Point& A,const Point& B) ->double{
    int64 delta_x = A.x - B.x;
    int64 delta_y = A.y - B.y;
    return sqrt(delta_x*delta_x + delta_y*delta_y);
};

inline auto cal_vec_angle = [](const Point& p) -> double{
    double angle = atan2(p.y,p.x);

    if(angle < 0)
    {
        angle += 2 * M_PI;
    }

    if(std::fabs(angle - 2*M_PI) < 1e-6)
    {
        angle = 0;
    }

    return angle;
};

void Nfp::processSegment(Segment* seg)
{
    Segkey key = this->getKey(seg);


    auto it = searchTable.find(key);

    if(it!=searchTable.end()) //说明存在相同角度的线段集合
    {
        vec_vec_seg[it->second].push_back(seg);
    }
    else
    {
        std::vector<Segment*> vecSeg;
        vecSeg.push_back(seg);

        int id = vec_vec_seg.size();
        vec_vec_seg.push_back(vecSeg);

        searchTable.insert({key,id});
    }
}

//A不动，B动。
void Nfp::nfp_line(Path A,Path B)
{
    A.erase(A.end()-1);
    B.erase(B.end()-1);

    Point vec_prev;
    Point vec_next;

    for(size_t i=0;i<B.size();i++)
    {
        int m = (i-1+B.size()) % B.size();
        int n = (i+1) % B.size();

        vec_prev = B[i] - B[m];  //当前索引前一条线段的向量
        vec_next = B[n] - B[i];  //当前索引所在线段的向量

        int64 cross = vec_prev.x * vec_next.y - vec_prev.y * vec_next.x;

        if(cross < 0) //凹点跳过
        {
            continue;
        }

        for(size_t j=0;j<A.size();j++)
        {
            int k = (j+1) % A.size();

            Point vec_curr = A[k] - A[j];

            vec_curr = cal_vec_norm(vec_curr);

            int64 value1 = vec_curr.x * vec_prev.x + vec_curr.y * vec_prev.y;
            int64 value2 = vec_curr.x * vec_next.x + vec_curr.y * vec_next.y;

            if (value1 <= 0 && value2 >= 0)
            {

                Point start = A[j] - B[i];
                Point end = A[k] - B[i];

                double len = cal_line_len(start,end);
                if(len < 1.0)
                {
                    continue;
                }

                //这里作重叠/反向处理
                Segment* seg = nullptr;

                if(g_use_obj_pool)
                {
                    Vert* v1 = ObjectPool<Vert>::getInstance().acquire(start);
                    Vert* v2 = ObjectPool<Vert>::getInstance().acquire(end);
                    seg = ObjectPool<Segment>::getInstance().acquire(v1,v2);
                }
                else
                {
                    Vert* v1 = new Vert(start);
                    Vert* v2 = new Vert(end);
                    seg = new Segment(v1,v2);
                }

                this->processSegment(seg);

            }
        }
    }

    for(size_t i=0;i<A.size();i++)
    {
        //qDebug() << "6";

        int m = (i-1+A.size()) % A.size();
        int n = (i+1) % A.size();

        vec_prev = A[i] - A[m];  //当前索引前一条线段的向量
        vec_next = A[n] - A[i];  //当前索引所在线段的向量

        int64 cross = vec_prev.x * vec_next.y - vec_prev.y * vec_next.x;

        if(cross < 0)
        {
            continue;
        }

        for(size_t j=0;j<B.size();j++)
        {
            //qDebug() << "8";
            int k = (j+1) % B.size();

            Point vec_curr = B[k] - B[j];

            vec_curr = cal_vec_norm(vec_curr);

            int64 value1 = vec_curr.x * vec_prev.x + vec_curr.y * vec_prev.y;
            int64 value2 = vec_curr.x * vec_next.x + vec_curr.y * vec_next.y;

            if (value1 <= 0 && value2 >= 0)
            {
                //qDebug() << "9";
                Point start = A[i] - B[j];
                Point end = A[i] - B[k];

                double len = cal_line_len(start,end);
                if(len < 1.0)
                {
                    continue;
                }

                //这里作重叠/反向处理
                Segment* seg = nullptr;

                if(g_use_obj_pool)
                {
                    Vert* v1 = ObjectPool<Vert>::getInstance().acquire(start);
                    Vert* v2 = ObjectPool<Vert>::getInstance().acquire(end);
                    seg = ObjectPool<Segment>::getInstance().acquire(v1,v2);
                }
                else
                {
                    Vert* v1 = new Vert(start);
                    Vert* v2 = new Vert(end);
                    seg = new Segment(v1,v2);
                }

                this->processSegment(seg);
            }
        }
    }

}

inline bool Nfp::vec_vec_seg2input()
{
    if(vec_vec_seg.empty())
    {
        qDebug() << "error::nfp::vec_vec_seg2input: vec_vec_seg is empty";
        return false;
    }

    for(std::vector<Segment*>& vec_seg : vec_vec_seg)
    {
        if(vec_seg.empty())
        {
            qDebug() << "error::nfp::vec_vec_seg2input: vec_seg is empty";
            return false;
        }

        for(Segment* seg : vec_seg)
        {
            if(seg == nullptr)
            {
                continue;
            }

            input.push_back(std::move(seg));
        }
    }

    return true;
}

bool Nfp::judgeLinkSegmentFromVerts(std::vector<Vert *> &verts)
{
    for(Vert* vert : verts)
    {
        if(vert->vec_start_point_segment.empty() || vert->vec_end_point_segment.empty()) 
        {
            continue;
        }

        QList<Segment*> list_seg;
        QList<bool> list_flag;
        QList<double> list_angle;   //记录线段的角度

        for(Segment* s : vert->vec_start_point_segment)
        {
            list_seg.append(s);
            list_flag.append(0);
            list_angle.append(s->angle);
        }

        for(Segment* s : vert->vec_end_point_segment)
        {
            list_seg.append(s);
            list_flag.append(1);

            double angle = s->angle;       

            double PI2 = M_PI * 2;

            angle -= M_PI;

            if(angle < 0)
            {
                angle += PI2;
            }

            if(qFabs(angle - PI2) < 1e-6)
            {
                angle = 0;
            }

            list_angle.append(angle);
        }

        bool flag = judgeLinkSegmentCurrentFromIntersection(list_seg,list_flag,list_angle);

        if(flag == false)
        {
            qDebug() << "error :nfp::judgeLinkVertFromIntersection: judgeLinkSegmentCurrentFromIntersection func ret value is false";
            return false;
        }
    }

    return true;
}

inline bool Nfp::judgeLinkSegmentCurrentFromIntersection(QList<Segment*>& list_seg, QList<bool>& list_flag, QList<double>& list_angle)
{
    //根据角度大小排序,升序
    for(int i=0;i<list_angle.size()-1;i++)
    {
        for(int j=i+1; j<list_angle.size();j++)
        {
            if(list_angle[i]>list_angle[j])
            {
                list_seg.swapItemsAt(i,j);
                list_flag.swapItemsAt(i,j);
                list_angle.swapItemsAt(i,j);
            }
        }
    }

    list_angle.append(list_angle.first());
    list_seg.append(list_seg.first());
    list_flag.append(list_flag.first());

    for(int i=0;i<list_flag.size()-1;i++)
    {
        if(list_flag[i] == 0)   //出边
        {
            continue;
        }

        int j = i+1;

        if(list_flag[j] == 1)   //入边
        {
            continue;
        }

        if(list_seg[i] == list_seg[j])  //线段重合
        {
            qDebug() << "error::nfp::judgeLinkSegmentCurrentFromIntersection:list_seg[i] == list_seg[j]";
            continue;
        }

        list_seg[i]->next = list_seg[j];

        list_seg[j]->prev = list_seg[i];
    }

    return true;
}

void Nfp::findPolygon(std::vector<Segment*>& vecSeg)
{
    Vert* minVert = vecSeg[0]->a;   //记录轮廓中y最小的点

    for(Segment* s : vecSeg)
    {
        //找出x最小的顶点
        if(s->a->pt.y < minVert->pt.y)
        {
            minVert = s->a;
        }

        if(s->b->pt.y < minVert->pt.y)
        {
            minVert = s->b;
        }

        if(s->isvalid == false)
        {
            continue;
        }

        if(s->prev == nullptr)
        {
            Segment* current = s;
            while(current != nullptr)
            {
                current->isvalid = false;
                current = current->next;
            }
        }
    }

    Segment *outside = nullptr; //外围轮廓中y值最小的线段

    QList<Segment*> list_seg_valid;
    for(Segment* s : minVert->vec_start_point_segment)
    {
        if(s->isvalid)
        {
            list_seg_valid.append(s);
        }
    }

    if(list_seg_valid.size() == 0)
    {
        //qDebug() << "error:nfp::findPolygon: list_seg_valid size is 0";
        return;
    }
    else if(list_seg_valid.size() == 1)
    {
        outside = list_seg_valid[0];
    }
    else if(list_seg_valid.size() > 1)
    {
        outside = list_seg_valid[0];
        for(int i=1;i<list_seg_valid.size();i++)
        {
            if(list_seg_valid[i]->angle < outside->angle)
            {
                outside = list_seg_valid[i];
            }
        }
    }


    QList<Segment*> list_allvalid_seg;

    //将所有有效线段提取到list_allvalid_seg
    for(Segment* s : vecSeg)
    {
        if(s->isvalid)
        {
            list_allvalid_seg.append(s);
        }
    }

    Path pathout;

    Segment* current = outside;

    for(int i=0;i<list_allvalid_seg.size();i++)
    {
        if(current && current->next != outside)
        {
            pathout.push_back(std::move(current->a->pt));
            current->isvalid = false;
            current = current->next;
        }
        else
        {
            if(current)
            {
                pathout.push_back(std::move(current->a->pt));
                current->isvalid = false;
            }
            break;
        }
    }

    pathout.push_back(pathout.front());
    m_result->push_back(pathout);

    //开始找孔洞
    while(!list_allvalid_seg.empty())
    {
        QList<Segment*> list_allvalid_temp;

        //将所有有效线段提取到list_allvalid
        for(Segment* s : list_allvalid_seg)
        {
            if(s->isvalid)
            {
                //qDebug() << s->a->pt.x << " " << s->a->pt.y << " " << s->b->pt.x << " " << s->b->pt.y;
                list_allvalid_temp.append(s);
            }
        }

        list_allvalid_seg.clear();

        list_allvalid_seg = list_allvalid_temp;

        list_allvalid_temp.clear();


        if(list_allvalid_seg.size() == 0)
        {
            //qDebug() << "nfp::findPolygon::no valid seg, all polygon have find";
            break;
        }

        Path pathhole;

        Segment* current = list_allvalid_seg[0];
        Segment* record = current;

        for(int i=0;i<list_allvalid_seg.size();i++)
        {
            if(current && current->next != record)
            {
                pathhole.push_back(std::move(current->a->pt));
                current->isvalid = false;
                current = current->next;
            }
            else
            {
                if(current)
                {
                    pathhole.push_back(std::move(current->a->pt));
                    current->isvalid = false;
                }
                break;
            }
        }

        pathhole.push_back(std::move(pathhole.front()));

        double area = Clipper2Lib::Area(pathhole);

        if(area < 0)
        {
            m_result->push_back(std::move(pathhole));
        }

    }

    if(m_result->size() == 0)
    {
        qDebug() << "error:nfp::findPolygon: no any polygon";
        return;
    }
}

void Nfp::run()
{
    this->nfp_line(*m_A,*m_B);  //轨迹线法计算所有可行线段，结果存储在成员变量vec_vec_seg中

    this->simplifySeg();

    bool flag = this->vec_vec_seg2input();
    if(!flag)
    {
        qDebug() << "nfp::nfp_run 1: vec_vec_seg2input ret false";
        return;
    }

    SweepLine sw;
    sw.run(this->input);

    flag = judgeLinkSegmentFromVerts(sw.verts);
    if(!flag)
    {
        qDebug() << "error:nfp::nfp_run 1: judgeLinkSegmentFromVerts ret is false";
        return;
    }

    findPolygon(sw.segments);
}









