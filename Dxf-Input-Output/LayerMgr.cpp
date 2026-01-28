#include "LayerMgr.h"

void LayerMgr::SetLayers(const DL_LayerData &data)
{
    layers.insert(data);
}

const std::set<DL_LayerData, LayerComp>& LayerMgr::GetLayers() const
{
    return layers;
}

