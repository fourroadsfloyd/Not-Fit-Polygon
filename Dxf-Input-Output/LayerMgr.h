#ifndef LAYERMGR_H
#define LAYERMGR_H

#include <set>

struct LayerComp {
    bool operator()(const DL_LayerData &a, const DL_LayerData &b) const {
        if (a.name < b.name) return true;
        if (b.name < a.name) return false;
        if (a.flags < b.flags) return true;
        if (b.flags < a.flags) return false;
        return (a.off < b.off);
    }
};


class LayerMgr{
public:
    static LayerMgr& GetInstance()
    {
        static LayerMgr lm;
        return lm;
    }

    ~LayerMgr() = default;

    void SetLayers(const DL_LayerData& data);

    // 返回对内部layer集合的只读引用，避免复制并保持自定义比较器类型一致
    const std::set<DL_LayerData, LayerComp>& GetLayers() const;

private:
    LayerMgr() = default;

    LayerMgr(const LayerMgr& other) = delete;  //注意类名中添加T
    LayerMgr& operator=(const LayerMgr& other) = delete;

private:
    std::set<DL_LayerData, LayerComp> layers;
};
#endif // LAYERMGR_H
