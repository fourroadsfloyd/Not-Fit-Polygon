#ifndef NFP_H
#define NFP_H

#include <vector>
#include <QElapsedTimer>


using Segkey = int;

class Nfp {
public:
    Nfp() = delete;

    Nfp(Path& A, Path& B, Paths& result);   //NFP存储在result中

    virtual ~Nfp() = default;

    //求解nfp
    void run();

protected:

    //1 求解轨迹线
    void nfp_line(Path A, Path B);
    void processSegment(Segment* seg);

    //2 处理vec_vec_seg中重复/反向线段
    void simplifySeg();
    inline Segkey getKey(Segment* seg);
    inline bool fast_test(Segment& s1,Segment& s2);
    inline int processOverlap(std::vector<Segment *> &vecSeg, int m, int n);

    //3 将vec_vec_seg中的Segment* 放到 Segment*
    inline bool vec_vec_seg2input();

    //4 根据扫描线中的Verts和Segments，将所有能够连接的线段连接起来
    bool judgeLinkSegmentFromVerts(std::vector<Vert*> &verts);//由交点判断线段是否链接
    inline bool judgeLinkSegmentCurrentFromIntersection(QList<Segment*>& list_seg, QList<bool>& list_flag, QList<double>& list_angle);

    //5 提取轮廓
    virtual void findPolygon(std::vector<Segment *> &vecSeg);


protected:

    std::vector<std::vector<Segment*>> vec_vec_seg; //不同角度的线段将创建不同的vector，并将其放置到vec_vec_seg中

    std::unordered_map<Segkey,int> searchTable; //key = 线段角度   value = 该线段所在vector容器在vec_vec_seg中的索引

    std::vector<Segment*> input; //存储经去重和去反向的线段集合,放到扫描线中

    Path* m_A = nullptr;
    Path* m_B = nullptr;

    Paths* m_result = nullptr;

    //====测试控制打印====
    bool debug_flag = false;

};



#endif // NFP_H
