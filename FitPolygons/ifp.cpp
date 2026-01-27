#include "ifp.h"

void Ifp::findPolygon(std::vector<Segment *> &vecSeg)
{
    for(Segment* s : vecSeg)
    {
        if(s->isvalid == false)
        {
            continue;
        }

        if(s->prev == nullptr)
        {
            Segment* current = s;
            while(current != nullptr)
            {
                //qDebug() << current->a->pt.x << " " << current->a->pt.y << " " << current->b->pt.x << " " << current->b->pt.y;
                current->isvalid = false;
                current = current->next;
            }
        }
    }

    QList<Segment*> list_allvalid_seg;

    //将所有有效线段提取到list_allvalid_seg
    for(Segment* s : vecSeg)
    {
        if(s->isvalid)
        {
            //qDebug() << s->a->pt.x << " " << s->a->pt.y << " " << s->b->pt.x << " " << s->b->pt.y;
            list_allvalid_seg.append(s);
        }
    }

    //开始找孔洞
    while(!list_allvalid_seg.empty())
    {
        QList<Segment*> list_allvalid_temp;

        //将所有有效线段提取到list_allvalid
        for(Segment* s : list_allvalid_seg)
        {
            if(s->isvalid)
            {
                //qDebug() << s->a->pt.x << " " << s->a->pt.y << " " << s->b->pt.x << " " << s->b->pt.y;
                list_allvalid_temp.append(s);
            }
        }

        list_allvalid_seg.clear();

        list_allvalid_seg = list_allvalid_temp;

        list_allvalid_temp.clear();


        if(list_allvalid_seg.size() == 0)
        {
            //qDebug() << "nfp::findPolygon::no valid seg, all polygon have find";
            break;
        }

        Path pathhole;

        Segment* current = list_allvalid_seg[0];
        Segment* record = current;

        for(int i=0;i<list_allvalid_seg.size();i++)
        {
            if(current && current->next != record)
            {
                pathhole.push_back(std::move(current->a->pt));
                current->isvalid = false;
                current = current->next;
            }
            else
            {
                if(current)
                {
                    pathhole.push_back(std::move(current->a->pt));
                    current->isvalid = false;
                }
                break;
            }
        }

        pathhole.push_back(std::move(pathhole.front()));

        double area = Clipper2Lib::Area(pathhole);

        if(area < 0)
        {
            m_result->push_back(std::move(pathhole));
        }

    }

    if(m_result->size() == 0)
    {
       // qDebug() << "error:ifp::findPolygon: no any polygon";
        return;
    }
}
