#include "fitpolygon.h"

FitPolygon::FitPolygon(Item &A_, Item &B_, bool AisSheet_)
{
    this->m_A = &A_;
    this->m_B = &B_;

    this->m_AisSheet = AisSheet_;
}

void FitPolygon::getNfpResult()
{
    if(m_AisSheet)  //A是板材,B是零件
    {
        Paths result;

        for(size_t i=0;i<m_A->m_holes.size();i++)
        {
            Nfp nfp(m_A->m_holes[i], m_B->m_contour, result);
            nfp.run();

            if(!result.empty())
            {
                nfpResult.insert(nfpResult.end(), result.begin(), result.end());
            }
        }
    }
    else    //A、B都是基本零件
    {
        Nfp nfp(m_A->m_contour,m_B->m_contour, nfpResult);
        nfp.run();


        //去除伪孔
        auto removeFakeHoles = [this](Path& path) -> bool{
            Item hole;
            hole.m_contour = path;
            hole.AABB();

            if(fabs(m_A->m_AABB.height() - m_B->m_AABB.height()) < hole.m_AABB.height() ||
                fabs(m_A->m_AABB.width() - m_B->m_AABB.width()) < hole.m_AABB.width())
            {
                return true;
            }

            //将B移动到孔洞上某点
            int64 dx = path[0].x - m_B->m_center.x;
            int64 dy = path[0].y - m_B->m_center.y;
            Item temp = *m_B;
            temp.move(dx,dy);

            Paths subject, clip, solution;

            subject.push_back(m_A->m_contour);

            clip.push_back(temp.m_contour);

            solution = Intersect(subject, clip, Clipper2Lib::FillRule::NonZero);

            if(!solution.empty())
            {
                return true;
            }

            return false;
        };

        nfpResult.erase(std::remove_if(nfpResult.begin()+1,nfpResult.end(),removeFakeHoles),nfpResult.end());

        // if(nfpResult.size() >1)
        // {
        //     qDebug()<<"m_A.id="<<m_A->m_id<<",m_A.m_AABB="<<m_A->m_AABB<<",m_A.area="<<m_A->m_area;
        //     qDebug()<<"m_B.id="<<m_B->m_id<<",m_B.m_AABB="<<m_B->m_AABB<<",m_B.area="<<m_B->m_area;
        // }
    }
}

void FitPolygon::getIfpResult()
{
    if(m_AisSheet)
    {
        if(m_A->m_AABB.width() < m_B->m_AABB.width() || m_A->m_AABB.height() < m_B->m_AABB.height()) return;
        if(std::abs(m_A->m_area) < m_B->m_area) return;


        Ifp ifp(m_A->m_contour, m_B->m_contour, ifpResult);
        ifp.run();
    }
    else
    {
        for(size_t i=0;i<m_A->m_holes.size();i++)
        {
             Paths result;

            Item temp;
            temp.m_contour = m_A->m_holes[i];
            temp.AABB();
            temp.Area();
            if(temp.m_AABB.width() < m_B->m_AABB.width() || temp.m_AABB.height() < m_B->m_AABB.height()) return;
            if(std::abs(temp.m_area) < m_B->m_area) return;

            Ifp ifp(m_A->m_holes[i], m_B->m_contour, result);
            ifp.run();


            if(!result.empty())
            {
                ifpResult.insert(ifpResult.end(), result.begin(), result.end());
            }

            // qDebug()<<"FitPolygon::getIFPResult: ifp.size="<<result.size()
            //          <<"ifp.size="<<ifpResult.size()<<",i="<<i;
        }
    }
}

void FitPolygon::getAllResult()
{
    if(m_AisSheet)
    {
        allResult.clear();

        ifpResult.clear();
        this->getIfpResult();

        if(ifpResult.empty())
        {
            return;
        }
        nfpResult.clear();
        this->getNfpResult();

        if(!ifpResult.empty())
        {
            std::reverse(ifpResult.begin(),ifpResult.end());
        }

        if(!nfpResult.empty())
        {
            for(Path& p : nfpResult)
            {
                std::reverse(p.begin(),p.end());
            }
        }

        allResult = Clipper2Lib::Difference(ifpResult,nfpResult,Clipper2Lib::FillRule::NonZero);

        for(Path& p : allResult)
        {
            if(p.front() != p.back())
            {
                p.push_back(p.front());
            }
        }

        //allResult = nfpResult;

        // for(Path& p : allResult)
        // {
        //     for(size_t i=0;i<p.size()-1;i++)
        //     {
        //         int j = i+1;
        //         qDebug() << p[i].x << " " << p[i].y << " " << p[j].x << " " << p[j].y;
        //     }
        // }

    }
    else
    {
        allResult.clear();

        nfpResult.clear();
        this->getNfpResult();
        if(!nfpResult.empty())
        {
            allResult.insert(allResult.end(),nfpResult.begin(),nfpResult.end());
        }else
        {
            qDebug()<<"error:no NFP in FitPolygon::getAllResult,m_A.ID="<<m_A->m_id_origin<<",m_B.id="<<m_B->m_id_origin
                     <<"A.rotateAngle="<<m_A->m_rotate_angle<<",B.rotateAngle="<<m_B->m_rotate_angle;

            // for(Point& p: m_A->m_contour)
            // {
            //     qDebug()<<"A:p.x="<<p.x<<",y="<<p.y;
            // }
            // for(Point& p: m_B->m_contour)
            // {
            //     qDebug()<<"B:p.x="<<p.x<<",y="<<p.y;
            // }
            return;
        }

        ifpResult.clear();
        this->getIfpResult();
        if(!ifpResult.empty())
        {
            allResult.insert(allResult.end(),ifpResult.begin(),ifpResult.end());
        }

        // qDebug()<<"FitPolygon::getAllResult: nfp.size="<<nfpResult.size()
        //          <<"ifp.size="<<ifpResult.size()<<",allresult.size="<<allResult.size();

        // for(Path& p : allResult)
        // {
        //     for(size_t i=0;i<p.size()-1;i++)
        //     {
        //         int j = i+1;
        //         qDebug() << p[i].x << " " << p[i].y << " " << p[j].x << " " << p[j].y;
        //     }
        // }
    }
}
