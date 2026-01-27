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

    uint16_t rotateIndex; 

    bool isMirror;              

    bool AisSheet;             
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

    NfpCache(Sheets &sheets_, Items &items_);     

    void run();
   
    Paths readItemAndItemNfp(ItemStates& A_, ItemStates& B_, int rotate_A_index, int rotate_B_index, bool ismirror_A, bool ismirror_B);

    Paths readSheetAndItemIfp(Item& A, Item& B, int rotate_B_index, bool ismirror_B);

private:
    Paths getNfpfromCache(Pair &pair);

    static void BuildFitPolygon(const Pair &pair);  

    static PairKey getPairKey(const Pair& pair);

    inline void MatchPair();                        
    inline void CreatePair(Item &A, ItemStates &itemstates_B, bool AisSheet);

    inline bool Compare(Item &A, Item &B);

private:
    Sheets* m_sheets;
    Items* m_items;

    std::vector<Pair> m_match_pairs;

    QFuture<void> m_future;
    QFutureWatcher<void> m_watcher; 

    Paths m_result;                          

signals:
    void nfpBuildFinished();
};

#endif // NFPCACHE_H
