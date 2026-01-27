#ifndef FITPOLYGON_H
#define FITPOLYGON_H

#include "item.h"

class FitPolygon
{
public:
    FitPolygon() = delete;

    FitPolygon(Item& A_, Item& B_, bool AisSheet_);

    void getNfpResult();

    void getIfpResult();

    void getAllResult();

    Paths nfpResult;
    Paths ifpResult;
    Paths allResult;

private:
    Item* m_A = nullptr;
    Item* m_B = nullptr;

    bool m_AisSheet = false;
};

#endif // FITPOLYGON_H
