#ifndef IFP_H
#define IFP_H

#include "nfp.h"

class Ifp : public Nfp{
public:
    Ifp() = delete;

    Ifp(Path& A_, Path& B_, Paths& result_) : Nfp(A_, B_, result_){}

    void findPolygon(std::vector<Segment *> &vecSeg) override;

};

#endif // IFP_H
