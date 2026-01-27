#include "nfpcache.h"
#include "fitpolygon.h"

NfpCache::NfpCache(Sheets &sheets_, Items &items_)
{
    this->m_sheets = &sheets_;
    this->m_items = &items_;

    this->m_match_pairs.reserve(100000);

    this->MatchPair();

    connect(&m_watcher, &QFutureWatcher<void>::finished,
            this, &NfpCache::nfpBuildFinished);
}

void NfpCache::CreatePair(Item& A, ItemStates& itemstates_B, bool AisSheet)
{
    std::vector<Item>& rotate_items = itemstates_B.m_rotate_items;

    if(!rotate_items.empty())
    {
        for(size_t i=0;i<rotate_items.size();i++)
        {
          //  Item& item = rotate_items[i];
            if(AisSheet || Compare(A,rotate_items[i]))
          {
                m_match_pairs.emplace_back(A, rotate_items[i], i, false, AisSheet);
            }

        }
    }

    std::vector<Item>& mirror_rotate_items = itemstates_B.m_mirror_rotate_items;

    if(!mirror_rotate_items.empty())
    {
        for(size_t i=0;i<mirror_rotate_items.size();i++)
        {
           // Item& item = mirror_rotate_items[i];
            if(AisSheet || Compare(A,mirror_rotate_items[i]))
            {
                m_match_pairs.emplace_back(A, mirror_rotate_items[i], i, true, AisSheet);
            }
        }
    }
}

bool NfpCache::Compare(Item &A, Item &B)
{

        if(A.m_id <= B.m_id)
           return true;


    return false;
}

void NfpCache::MatchPair()
{
    for(SheetStates& sheetstates : *m_sheets)
    {
        Item& sheet = sheetstates.m_item_simple;

        for(ItemStates& itemstates : *m_items)
        {
            CreatePair(sheet, itemstates, true);
        }
    }

    for(size_t i=0;i<m_items->size();i++)
    {
        ItemStates& itemstates_A = (*m_items)[i];

        Item& A = itemstates_A.m_item_simple;

        for(size_t j=0;j<m_items->size();j++)
        {
            CreatePair(A, (*m_items)[j], false);
        }
    }
}

PairKey NfpCache::getPairKey(const Pair &pair)
{
    return (static_cast<uint64_t>(pair.A->m_id)           ) |           // [15: 0]
           (static_cast<uint64_t>(pair.B->m_id)     << 16 ) |           // [31: 16]
           (static_cast<uint64_t>(pair.rotateIndex) << 32 ) |           // [47:31]
           (static_cast<uint64_t>(pair.isMirror)    << 49 ) |           // [48]
           (static_cast<uint64_t>(pair.AisSheet)    << 50 );            // [49]
}

void NfpCache::BuildFitPolygon(const Pair &pair)
{
    if(g_use_obj_pool)
    {
        //初始化对象池
        auto& segPool = ObjectPool<Segment>::getInstance();
        segPool.initialize(ysk::SEGMENT_PER_THREAD);

        auto& vertPool = ObjectPool<Vert>::getInstance();
        vertPool.initialize(ysk::VERT_PER_THREAD);
    }

    PairKey key = getPairKey(pair);

    FitPolygon fitpolygon(*pair.A,*pair.B,pair.AisSheet);

    fitpolygon.getAllResult();
    if(fitpolygon.allResult.size() == 0)
    {
        AsyncLog::getInstance().log(LogLevel::ERROR,
                                    "NfpCache::BuildFitPolygon:"
                                    "A.id={}, B.id={}, B.rot.index={}, B.ismirror={}",
                                    pair.A->m_id_origin, pair.B->m_id_origin, pair.rotateIndex,
                                    pair.isMirror, pair.AisSheet);
        return;
    }

    g_cache_mutex.lock();
    g_NFP_Cache[key] = std::move(fitpolygon.allResult);
    g_cache_mutex.unlock();
}

std::unordered_map<PairKey, Paths> g_NFP_Cache;
std::mutex g_cache_mutex;

void NfpCache::run()
{
    g_NFP_Cache.reserve(100000);

    g_use_mem_pool = true;  //开启内存池
    g_use_obj_pool = true;  //开启对象池

    AsyncLog::getInstance().log(LogLevel::INFO, "start construct NFP");

    this->m_future = QtConcurrent::map(this->m_match_pairs, &NfpCache::BuildFitPolygon);    //线程池开始工作
    this->m_watcher.setFuture(m_future);
}

Paths NfpCache::readItemAndItemNfp(ItemStates& A_, ItemStates& B_, int rotate_A_index, int rotate_B_index, bool ismirror_A, bool ismirror_B)
{
    m_result.clear();

    Pair pair;

    Item&  A = A_.m_item_simple;

    Item& B = B_.m_item_simple;

    if(ismirror_A)
    {
    //A镜像
        if(Compare(A,B))
        {
            pair.A = &A;
            pair.B = &B;
            pair.AisSheet = false;  //没有板材

            if(ismirror_B)
            {
                pair.isMirror = 0;
            }else
            {
                pair.isMirror = 1;
            }

            int cout = B_.m_rotate_items.size();

            int rotate_B = rotate_A_index - rotate_B_index;
            if(rotate_B < 0)
            {
                rotate_B += cout;
            }
            pair.rotateIndex = (uint16_t)rotate_B;

            PairKey key = this->getPairKey(pair);

            auto it = g_NFP_Cache.find(key);

            if(it == g_NFP_Cache.end())
            {
                qDebug() << "error::NfpCache::readItemAndItemNfp: (1) no find nfp"
                         <<",pair.A.id="<<pair.A->m_id<<",pair.B.id="<<pair.B->m_id
                         <<"pair.ismirror="<<pair.isMirror<<",pair.rotid="<<pair.rotateIndex
                         <<"pair.AisSheet="<<pair.AisSheet;
                return Paths{};
            }

            Paths path = it->second;
            path = my_paths_mirror(path);

            my_paths_rotate(path,360.0/cout*rotate_A_index);

            return path;

        }
        else
        {
            pair.A = &B;
            pair.B = &A;
            pair.AisSheet = false;

            if(ismirror_B)
            {
                pair.isMirror = 0;

                int cout = B_.m_rotate_items.size();

                int rotate_B = rotate_B_index - rotate_A_index;
                if(rotate_B < 0)
                {
                    rotate_B += cout;
                }
                pair.rotateIndex = (uint16_t)rotate_B;

                PairKey key = this->getPairKey(pair);

                auto it = g_NFP_Cache.find(key);

                if(it == g_NFP_Cache.end())
                {
                    qDebug() << "error::NfpCache::readItemAndItemNfp: (2) no find nfp"
                             <<",pair.A.id="<<pair.A->m_id<<",pair.B.id="<<pair.B->m_id
                             <<"pair.ismirror="<<pair.isMirror<<",pair.rotid="<<pair.rotateIndex
                             <<"pair.AisSheet="<<pair.AisSheet;
                    return Paths{};
                }

                Paths path = it->second;

                    path = my_paths_mirror(path);

                my_paths_rotate(path,360.0/cout*rotate_B_index - 180.0);
                return path;
            }else
            {
                pair.isMirror = 1;

                int cout = B_.m_rotate_items.size();

                int rotate_B = rotate_A_index - rotate_B_index;
                if(rotate_B < 0)
                {
                    rotate_B += cout;
                }
                pair.rotateIndex = (uint16_t)rotate_B;

                PairKey key = this->getPairKey(pair);

                auto it = g_NFP_Cache.find(key);

                if(it == g_NFP_Cache.end())
                {
                    qDebug() << "error::NfpCache::readItemAndItemNfp: (3) no find nfp"
                             <<",pair.A.id="<<pair.A->m_id<<",pair.B.id="<<pair.B->m_id
                             <<"pair.ismirror="<<pair.isMirror<<",pair.rotid="<<pair.rotateIndex
                             <<"pair.AisSheet="<<pair.AisSheet;
                    return Paths{};
                }

                Paths path = it->second;

               // path = my_paths_mirror(path);

                my_paths_rotate(path,360.0/cout*rotate_B_index - 180.0);
                return path;
            }
        }

    }else
    {
        //A不镜像

        if(Compare(A,B))
        {
            pair.A = &A;
            pair.B = &B;
            pair.AisSheet = false;  //没有板材

            if(ismirror_B)
            {
                pair.isMirror = 1;
            }else
            {
                pair.isMirror = 0;
            }

            int cout = B_.m_rotate_items.size();

            int rotate_B = rotate_B_index - rotate_A_index;
            if(rotate_B < 0)
            {
                rotate_B += cout;
            }
            pair.rotateIndex = (uint16_t)rotate_B;

             PairKey key = this->getPairKey(pair);

            auto it = g_NFP_Cache.find(key);

            if(it == g_NFP_Cache.end())
            {
                qDebug() << "error::NfpCache::readItemAndItemNfp: (4) no find nfp"
                         <<",pair.A.id="<<pair.A->m_id<<",pair.B.id="<<pair.B->m_id
                         <<"pair.ismirror="<<pair.isMirror<<",pair.rotid="<<pair.rotateIndex
                         <<"pair.AisSheet="<<pair.AisSheet;
                return Paths{};
            }

            Paths path = it->second;
            my_paths_rotate(path,360.0/cout*rotate_A_index);
            return path;

        }
        else
        {
            pair.A = &B;
            pair.B = &A;
            pair.AisSheet = false;

            if(ismirror_B)
            {
                pair.isMirror = 1;

                int cout = B_.m_rotate_items.size();

                int rotate_B = rotate_B_index - rotate_A_index;
                if(rotate_B < 0)
                {
                    rotate_B += cout;
                }
                pair.rotateIndex = (uint16_t)rotate_B;

                PairKey key = this->getPairKey(pair);

                auto it = g_NFP_Cache.find(key);

                if(it == g_NFP_Cache.end())
                {
                    qDebug() << "error::NfpCache::readItemAndItemNfp: (5) no find nfp"
                             <<",pair.A.id="<<pair.A->m_id<<",pair.B.id="<<pair.B->m_id
                             <<"pair.ismirror="<<pair.isMirror<<",pair.rotid="<<pair.rotateIndex
                             <<"pair.AisSheet="<<pair.AisSheet;
                    return Paths{};
                }

                Paths path = it->second;

                    path = my_paths_mirror(path);

                my_paths_rotate(path,360.0/cout*rotate_B_index - 180.0);
                return path;
            }else
            {
                pair.isMirror = 0;

                int cout = B_.m_rotate_items.size();

                int rotate_B = rotate_A_index - rotate_B_index;
                if(rotate_B < 0)
                {
                    rotate_B += cout;
                }
                pair.rotateIndex = (uint16_t)rotate_B;

                PairKey key = this->getPairKey(pair);

                auto it = g_NFP_Cache.find(key);

                if(it == g_NFP_Cache.end())
                {
                    qDebug() << "error::NfpCache::readItemAndItemNfp: (6) no find nfp"
                             <<",pair.A.id="<<pair.A->m_id<<",pair.B.id="<<pair.B->m_id
                             <<"pair.ismirror="<<pair.isMirror<<",pair.rotid="<<pair.rotateIndex
                             <<"pair.AisSheet="<<pair.AisSheet;
                    return Paths{};
                }

                Paths path = it->second;
                my_paths_rotate(path,360.0/cout*rotate_B_index - 180.0);
                return path;
            }
        }
    }

    return Paths{};
}

Paths NfpCache::readSheetAndItemIfp(Item& A, Item& B, int rotate_B_index, bool ismirror_B)
{
    m_result.clear();

    Pair pair;


        pair.A = &A;
        pair.B = &B;
        pair.AisSheet = true;   //pair.A是板材
        pair.isMirror = ismirror_B;
        pair.rotateIndex = rotate_B_index;


        PairKey key = this->getPairKey(pair);

        auto it = g_NFP_Cache.find(key);

        if(it == g_NFP_Cache.end())
        {
            return Paths{};
        }

        return it->second;
}

inline auto Paths2Item = [](Paths& path, Item& item)->void{         //注意，hash表中的数据仅能拷贝，禁止std::move
    item.m_contour = path[0];
    item.m_holes.assign(path.begin()+1, path.end());
};

inline auto Item2Paths = [](Item& item, Paths& path)->void{
    path.push_back(std::move(item.m_contour));
    path.insert(path.end(),
                    std::make_move_iterator(item.m_holes.begin()),
                    std::make_move_iterator(item.m_holes.end()));
};

Paths NfpCache::getNfpfromCache(Pair& pair)
{

    PairKey key = this->getPairKey(pair);

    auto it = g_NFP_Cache.find(key);

    if(it == g_NFP_Cache.end())
    {
        qDebug() << "error::NfpCache::readItemAndItemNfp: (8) no find nfp"
                 <<",pair.A.id="<<pair.A->m_id<<",pair.B.id="<<pair.B->m_id
                 <<"pair.ismirror="<<pair.isMirror<<",pair.rotid="<<pair.rotateIndex
                 <<"pair.AisSheet="<<pair.AisSheet;
        return Paths{};
    }

  return it->second;
}





