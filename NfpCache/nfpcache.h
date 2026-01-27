#ifndef NFPCACHE_H
#define NFPCACHE_H

#include <QWidget>
#include <QFuture.h>
#include <QFutureWatcher>
#include <QElapsedTimer>
#include <mutex>
#include <unordered_map>
#include "item.h"


struct Pair{
    Pair() = default;

    Pair(Item& A_, Item& B_, int rotateIndex_, bool isMirror_, bool AisSheet_)
        : A(&A_),B(&B_),rotateIndex(rotateIndex_),isMirror(isMirror_),AisSheet(AisSheet_){}

    Item* A = nullptr;
    Item* B = nullptr;

    uint16_t rotateIndex;       //存储B的vector索引号,if isMirror==false,就是rotate_items中的索引，else 就是mirror_rotate_items中的索引

    bool isMirror;              //B是镜像吗?

    bool AisSheet;              //A是板材吗?
};

using PairKey = uint64_t;

extern std::unordered_map<PairKey, Paths> g_NFP_Cache;

extern std::mutex g_cache_mutex;

inline bool g_use_mem_pool{false};

inline bool g_use_obj_pool{false};

class NfpCache : public QObject{
    Q_OBJECT
public:
    NfpCache() = delete;

    NfpCache(Sheets &sheets_, Items &items_);       //获取板材和零件信息

    void run();

    //返回的Paths,外边缘是逆时针，孔洞是顺时针，A,B都是旋转队列第1个，即原始件。
    //B碰A
    Paths readItemAndItemNfp(ItemStates& A_, ItemStates& B_, int rotate_A_index, int rotate_B_index, bool ismirror_A, bool ismirror_B);

    //返回的Paths,外边缘是逆时针，孔洞是顺时针。需注意对于板材而言，逆时针的左侧有效，顺时针的右侧有效
    //A 是sheet板
    Paths readSheetAndItemIfp(Item& A, Item& B, int rotate_B_index, bool ismirror_B);

private:
    Paths getNfpfromCache(Pair &pair);

    static void BuildFitPolygon(const Pair &pair);  //传入线程池，Pair的处理函数

    static PairKey getPairKey(const Pair& pair);

    inline void MatchPair();                        //板材和零件配对，零件和零件配对，配对结果保存在m_match_pairs中，需注意，pair.A永远是面积最大的
    inline void CreatePair(Item &A, ItemStates &itemstates_B, bool AisSheet);

    inline bool Compare(Item &A, Item &B);

private:
    Sheets* m_sheets;//必须编号 Item.m_id,m_area必须提前计算好
    Items* m_items;//必须编号 Item.m_id，m_area必须提前计算好


    std::vector<Pair> m_match_pairs;

    QFuture<void> m_future;
    QFutureWatcher<void> m_watcher;  // 监听异步任务完成

    Paths m_result;                                 //外部read得到的NFP

signals:
    void nfpBuildFinished();
};

#endif // NFPCACHE_H
