#ifndef NFP_H
#define NFP_H

#include <vector>
#include <QElapsedTimer>


using Segkey = int;

class Nfp {
public:
    Nfp() = delete;

    Nfp(Path& A, Path& B, Paths& result); 
    virtual ~Nfp() = default;

    //求解nfp
    void run();

protected:
    void nfp_line(Path A, Path B);
    void processSegment(Segment* seg);

    void simplifySeg();
    inline Segkey getKey(Segment* seg);
    inline bool fast_test(Segment& s1,Segment& s2);
    inline int processOverlap(std::vector<Segment *> &vecSeg, int m, int n);

    inline bool vec_vec_seg2input();

    bool judgeLinkSegmentFromVerts(std::vector<Vert*> &verts);
    inline bool judgeLinkSegmentCurrentFromIntersection(QList<Segment*>& list_seg, QList<bool>& list_flag, QList<double>& list_angle);

    virtual void findPolygon(std::vector<Segment *> &vecSeg);


protected:

    std::vector<std::vector<Segment*>> vec_vec_seg;

    std::unordered_map<Segkey,int> searchTable;

    std::vector<Segment*> input; 

    Path* m_A = nullptr;
    Path* m_B = nullptr;

    Paths* m_result = nullptr;

    //====测试控制打印====
    bool debug_flag = false;

};

#endif // NFP_H
