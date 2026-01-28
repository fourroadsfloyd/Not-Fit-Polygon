#include <QDebug>
#include "mycreationdxf.h"

//------------------------------------------------无用--------------------------------------------------------
//===========================================================================================================
void MyCreationDxf::addLayer(const DL_LayerData& data)
{
    LayerMgr::GetInstance().SetLayers(data);    //单例类保存所有图层信息，set容器自动去重，保证图层唯一性
}

//添加线段
void MyCreationDxf::addLine(const DL_LineData& data)
{
    LineData linedata;
    linedata.x1 = data.x1;
    linedata.y1 = data.y1;
    linedata.x2 = data.x2;
    linedata.y2 = data.y2;
    linedata.layer = attributes.getLayer();

    my_math_computer_Line_S_E_Point(linedata);

    this->dxf_data.line_data.append(linedata);
}

//添加圆弧
void MyCreationDxf::addArc(const DL_ArcData& data)
{
    ArcData arcdata;
    arcdata.cx = data.cx;
    arcdata.cy = data.cy;
    arcdata.cz = data.cz;
    arcdata.radius = data.radius;
    arcdata.angle1 = data.angle1;
    arcdata.angle2 = data.angle2;

    my_math_computer_Arc_S_E_Point(arcdata);    //my_math中的函数,计算圆弧的起点和终点坐标
    my_math_computer_Arc_bulge(arcdata);

    this->dxf_data.arc_data.append(arcdata);
}

//添加圆
void MyCreationDxf::addCircle(const DL_CircleData& data)
{
    CircleData cdata;
    cdata.cx = data.cx;
    cdata.cy = data.cy;
    cdata.cz = data.cz;
    cdata.radius = data.radius;
    cdata.layer = attributes.getLayer();

    this->dxf_data.circle_data.append(cdata);
}

//添加多边形
void MyCreationDxf::addPolyline(const DL_PolylineData& data)
{
    PolylineData polylinedata;
    polylinedata.number = data.number;
    polylinedata.flags = data.flags;
    polylinedata.layer = attributes.getLayer();

    this->dxf_data.polyline_data.append(polylinedata);
}

//添加多边形的顶点
void MyCreationDxf::addVertex(const DL_VertexData& data)
{
    PolylineData& last_it = dxf_data.polyline_data.back();  //获取polyline_data链表中最后一个元素的引用。调用addVertex前必定调用了addPolyline。注意:polyline_data一定不为空

    last_it.vertexdata.append(data);
}

//获取私有成员变量dxf_data
DxfData& MyCreationDxf::getDxfData()
{
    return this->dxf_data;
}

//将Line和Arc转化为Polyline
void MyCreationDxf::TransformLineAndArcToPolyline()
{
    if(getDxfData().line_data.isEmpty() && getDxfData().arc_data.isEmpty())
    {
        return;
    }

    // 处理线段 + 圆弧
    if (!getDxfData().line_data.isEmpty() && !getDxfData().arc_data.isEmpty())
    {     
        processLinesAndArcs();
        processOnlyArc();
    }
    // 处理只有线段
    else if (!getDxfData().line_data.isEmpty())
    {
        processOnlyLine();
    }
    // 处理只有圆弧
    else if (!getDxfData().arc_data.isEmpty())
    {
        printf("arc\n");
        processOnlyArc();
    }
}

void MyCreationDxf::processLinesAndArcs() //先从线段中开始搜索
{
    QList<LineData>& list_line = this->getDxfData().line_data;
    QList<ArcData>& list_arc = this->getDxfData().arc_data;
    QList<LineData> list_recover_line; //当不能组成封闭图形时，将链表恢复到原先状态，主要是恢复flag标志位
    QList<ArcData> list_recover_arc;
    QList<DL_VertexData> list_vertex;
    PolylineData polylinedata;

    bool hadClosed;
    bool foundLinkPoint;

    QPointF startPoint;
    QPointF linkPoint;

    for (int i = 0; i < list_line.size(); i++) {
        if (list_line[i].flag != 0) continue;

        list_recover_line = list_line;
        list_recover_arc = list_arc;

        hadClosed = false;
        foundLinkPoint = false;
        startPoint = list_line[i].startP;
        // 添加起点
        list_vertex.append(update_vertexdata(startPoint,0));
        list_line[i].flag = 1;

        linkPoint = list_line[i].endP;

        while (!hadClosed) {

            // 查找线段连接点
            foundLinkPoint = findNextLineLink(list_line, linkPoint, list_vertex,startPoint, hadClosed);

            // 查找圆弧连接点
            if (!foundLinkPoint) {

                foundLinkPoint = findNextArcLink(list_arc, linkPoint, list_vertex,startPoint, hadClosed);
            }

            // 未找到连接点
            if (!foundLinkPoint && !hadClosed)
            {
                list_line = list_recover_line;
                list_arc = list_recover_arc;
                //qDebug() << "未封闭图形! 最后点:" << linkPoint;
                break;
            }
        }

        if(hadClosed)
        {
            polylinedata.number = list_vertex.size();
            polylinedata.flags = 1;
            polylinedata.vertexdata = list_vertex;
            this->dxf_data.polyline_data.append(polylinedata);
        }
        list_vertex.clear();
    }
}

/*
 * author:袁世宽
 * date:2025-6-13
 */
//1 让封闭图形顶点链表中的末点等于起点
//2 让封闭图形的flags==1(图形闭合的标志位)
//3 在line和Arc中找到使polyline封闭的顶点
void MyCreationDxf::ProcessPolyline()
{
    QList<PolylineData>& list_polyline = this->getDxfData().polyline_data;

    for(int i=0;i<list_polyline.size();i++)
    {
        //当polyline中的起点和末点不相等，但flags==1,则在顶点链表末尾添加起点数据，使得起末相等
        if((std::abs(list_polyline[i].vertexdata.first().x - list_polyline[i].vertexdata.last().x) > 1e-6
          || std::abs(list_polyline[i].vertexdata.first().y - list_polyline[i].vertexdata.last().y) > 1e-6) && list_polyline[i].flags == 1)
        {
            list_polyline[i].vertexdata.append(list_polyline[i].vertexdata.first());
        }

        //当polyline中的起点和末点相等，但flags != 1,则将flags设为1
        if((std::abs(list_polyline[i].vertexdata.first().x - list_polyline[i].vertexdata.last().x) < 1e-6
          || std::abs(list_polyline[i].vertexdata.first().y - list_polyline[i].vertexdata.last().y) < 1e-6) && list_polyline[i].flags !=1)
        {
            list_polyline[i].flags =1;
        }
    }

    QList<LineData>& list_line = this->getDxfData().line_data;
    QList<ArcData>& list_arc = this->getDxfData().arc_data;
    QList<LineData> list_recover_line; //当不能组成封闭图形时，将链表恢复到原先状态，主要是恢复flag标志位
    QList<ArcData> list_recover_arc;
    QList<DL_VertexData> list_vertex;

    bool hadClosed;
    bool foundLinkPoint;

    QPointF startPoint;
    QPointF linkPoint;

    for (int i = 0; i < list_polyline.size(); i++) {
        if (list_polyline[i].flags != 0) continue;

        hadClosed = false;
        foundLinkPoint = false;
        startPoint = QPointF(list_polyline[i].vertexdata.first().x,list_polyline[i].vertexdata.first().y);

        linkPoint = QPointF(list_polyline[i].vertexdata.last().x,list_polyline[i].vertexdata.last().y);

        list_recover_line = list_line;
        list_recover_arc = list_arc;

        while (!hadClosed) {
            // 查找线段连接点
            foundLinkPoint = findNextLineLink(list_line, linkPoint, list_vertex,startPoint, hadClosed);

            // 查找圆弧连接点
            if (!foundLinkPoint) {

                foundLinkPoint = findNextArcLink(list_arc, linkPoint, list_vertex,startPoint, hadClosed);
            }

            //list_arc = list_recover_arc;
            // 未找到连接点
            if (!foundLinkPoint && !hadClosed)
            {
                list_line = list_recover_line;
                list_arc = list_recover_arc;
                qDebug() << "未封闭图形! 最后点:" << linkPoint;
                break;
            }
        }

        if(hadClosed)
        {
            //与线段/圆弧向polyline中塞数据不同，第一个linkpoint已经在polyline中了。polyline中的顶点链表的末点只需要接收line和arc组成的顶点链表的起点的bulge值。
            list_polyline[i].vertexdata.last().bulge = list_vertex.first().bulge;
            list_vertex.removeFirst();  //拼接前，删除第一个点，防止出现重复

            list_polyline[i].vertexdata.append(list_vertex);
            list_polyline[i].number = (int)list_polyline[i].number + (int)list_vertex.size();
            list_polyline[i].flags = 1;
        }
        list_vertex.clear();
    }
}

void MyCreationDxf::processOnlyLine() {
    QList<LineData>& list_line = getDxfData().line_data;
    QList<LineData> list_recover_line; //当不能组成封闭图形时，将链表恢复到原先状态，主要是恢复flag标志位
    QList<DL_VertexData> list_vertex;
    PolylineData polylinedata;

    bool hadClosed;
    bool foundLinkPoint;

    QPointF startPoint;
    QPointF linkPoint;

    for (int i = 0; i < list_line.size(); i++) {
        if (list_line[i].flag != 0) continue;

        list_recover_line = list_line;

        hadClosed = false;
        foundLinkPoint = false;
        startPoint = list_line[i].startP;      
        // 添加起点
        list_vertex.append(update_vertexdata(startPoint,0));
        list_line[i].flag = 1;

        linkPoint = list_line[i].endP;

        while (!hadClosed) {
            // 查找线段连接点
            foundLinkPoint = findNextLineLink(list_line, linkPoint, list_vertex,startPoint, hadClosed);

            // 未找到连接点
            if (!foundLinkPoint && !hadClosed) {
                list_line = list_recover_line;
                //qDebug() << "未封闭图形! 最后点:" << linkPoint;
                break;
            }
        }

        if(hadClosed)
        {
            polylinedata.number = list_vertex.size();
            polylinedata.flags = 1;
            polylinedata.vertexdata = list_vertex;
            this->dxf_data.polyline_data.append(polylinedata);
        }
        list_vertex.clear();
    }
}

void MyCreationDxf::processOnlyArc() {
    QList<ArcData>& list_arc = getDxfData().arc_data;
    QList<ArcData> list_recover_arc;
    QList<DL_VertexData> list_vertex;
    PolylineData polylinedata;

    bool hadClosed;
    bool foundLinkPoint;

    QPointF startPoint;
    QPointF linkPoint;

    for (int i = 0; i < list_arc.size(); i++) {

        if (list_arc[i].flag != 0) continue;

        list_recover_arc = list_arc;

        hadClosed = false;
        foundLinkPoint = false;
        startPoint = list_arc[i].startP;
        // 添加起点
        list_vertex.append(update_vertexdata(startPoint,list_arc[i].bulge));
        list_arc[i].flag = 1;

        linkPoint = list_arc[i].endP;

        while (!hadClosed) {
            // 查找线段连接点
            foundLinkPoint = findNextArcLink(list_arc, linkPoint,list_vertex,startPoint, hadClosed);

            // 未找到连接点
            if (!foundLinkPoint && !hadClosed) {
                list_arc = list_recover_arc;
                //qDebug() << "未封闭图形! 最后点:" << linkPoint;
                break;
            }
        }

        if(hadClosed)
        {
            polylinedata.number = list_vertex.size();
            polylinedata.flags = 1;
            polylinedata.vertexdata = list_vertex;
            this->dxf_data.polyline_data.append(polylinedata);
        }
        list_vertex.clear();
    }
}

bool MyCreationDxf::findNextLineLink(QList<LineData>& list_line,QPointF& linkPoint,
                                     QList<DL_VertexData>& list_vertex,QPointF& startPoint,bool& hadClosed)
{
    for(int i = 0;i<list_line.size();i++)
    {
        if(list_line[i].flag != 0)
        {
            continue;
        }

        if(qAbs(linkPoint.x() - list_line[i].startP.x()) < 1e-5 && qAbs(linkPoint.y() - list_line[i].startP.y()) < 1e-5)
        {
            list_vertex.append(update_vertexdata(linkPoint,0));

            linkPoint = list_line[i].endP;
            list_line[i].flag = 1;

            if(qAbs(linkPoint.x() - startPoint.x()) < 1e-5 && qAbs(linkPoint.y() - startPoint.y()) < 1e-5)
            {
                list_vertex.append(update_vertexdata(linkPoint,0));

                hadClosed = true;
            }

            return true;
        }
        else if(qAbs(linkPoint.x() - list_line[i].endP.x()) < 1e-5 && qAbs(linkPoint.y() - list_line[i].endP.y()) < 1e-5)
        {
            list_vertex.append(update_vertexdata(linkPoint,0));

            linkPoint = list_line[i].startP;
            list_line[i].flag = 1;

            if(qAbs(linkPoint.x() - startPoint.x()) < 1e-5 && qAbs(linkPoint.y() - startPoint.y()) < 1e-5)
            {
                list_vertex.append(update_vertexdata(linkPoint,0));

                hadClosed = true;
            }

            return true;
        }
    }
    return false;
}

bool MyCreationDxf::findNextArcLink(QList<ArcData>& list_arc,QPointF& linkPoint,
                                     QList<DL_VertexData>& list_vertex,QPointF& startPoint,bool& hadClosed)
{

    for(int i=0; i < list_arc.size(); i++)
    {

        if(list_arc[i].flag != 0)
        {
            continue;
        }

        if(qAbs(linkPoint.x() - list_arc[i].startP.x()) < 1e-5 && qAbs(linkPoint.y() - list_arc[i].startP.y()) < 1e-5)
        {
            //printf("in qabs\n");
            //在找到新的连接点的时候才将老的连接点插入到链表中，是为了保持顶点中只有圆弧的起点才存储凸度
            list_vertex.append(update_vertexdata(linkPoint,list_arc[i].bulge));

            linkPoint = list_arc[i].endP;   //将老的连接点更新成新的连接点
            list_arc[i].flag = 1;

            if(qAbs(linkPoint.x() - startPoint.x()) < 1e-5 && qAbs(linkPoint.y() - startPoint.y()) < 1e-5)
            {
                list_vertex.append(update_vertexdata(linkPoint,0));

                hadClosed = true;
            }

            return true;

        }
        else if(qAbs(linkPoint.x() - list_arc[i].endP.x()) < 1e-5 && qAbs(linkPoint.y() - list_arc[i].endP.y()) < 1e-5)
        {
            list_vertex.append(update_vertexdata(linkPoint,-list_arc[i].bulge));    //若连接点是末点，则圆弧的凸度应该取反

            //printf("list_arc[i].bulge:%6.3f\n",list_arc[i].bulge);

            linkPoint = list_arc[i].startP;
            list_arc[i].flag = 1;

            if(qAbs(linkPoint.x() - startPoint.x()) < 1e-5 && qAbs(linkPoint.y() - startPoint.y()) < 1e-5)
            {
                list_vertex.append(update_vertexdata(linkPoint,0));

                hadClosed = true;
            }

            return true;

        }
    }
    return false;
}

DL_VertexData MyCreationDxf::update_vertexdata(QPointF linkPoint,double bulge)
{
    DL_VertexData vertexdata;
    vertexdata.x = linkPoint.x();
    vertexdata.y = linkPoint.y();
    vertexdata.bulge = bulge;
    return vertexdata;
}

double MyCreationDxf::computer_Area(QList<DL_VertexData> vertexdata)
{
    double sum1 = 0;
    double sum2 = 0;

    for(int i=0;i<vertexdata.size();i++)
    {
        int j = (i + 1) % vertexdata.size(); //循环队列判断公式
        sum1 += vertexdata[i].x * vertexdata[j].y;
        sum2 += vertexdata[i].y * vertexdata[j].x;
    }

     //printf("\nArea:%6.3f\n",sum1-sum2);
    return sum1 - sum2;
}

void MyCreationDxf::TransformCCW(QList<DL_VertexData>& vertexdata)
{

//    for(int i = 0;i<vertexdata.size();i++)
//    {
//        printf("=============i:%d==============\n",i);
//        printf("point:(%6.3f,%6.3f)\n",vertexdata[i].x,vertexdata[i].y);
//        printf("bulge:%6.3f\n",vertexdata[i].bulge);
//    }

    if(vertexdata.size() == 3)      //(两个圆弧)或(直线-圆弧)两个点组成的封闭图形，若使用鞋带公式会使得面积=0,后续无法判断是否逆时针,这里进行特殊处理。
    {

        if(vertexdata[0].bulge>0||vertexdata[1].bulge>0)    //根据圆弧判断顺逆方向。如果圆弧的凸度>0,则一定是逆时针
        {
            return;
        }

        if(vertexdata[0].bulge < 0)                         //如果圆弧的凸度<0,交换凸度值，并取反。
        {
            vertexdata[1].bulge = -vertexdata[0].bulge;
            vertexdata[0].bulge = 0;
        }
        else if(vertexdata[1].bulge < 0)
        {
            vertexdata[0].bulge = -vertexdata[1].bulge;
            vertexdata[1].bulge = 0;
        }
        return;
    }

    double area = computer_Area(vertexdata);
    //printf("area:%6.3f\n",area);
    if (area > 0)
    {
        return;
    }
    else
        std::reverse(vertexdata.begin(),vertexdata.end());  //反转链表

    // 处理所有 bulge 值
    for(int i = 0; i < vertexdata.size(); i++) {
        if(vertexdata[i].bulge!=0)
            vertexdata[i].bulge = -vertexdata[i].bulge;


        if(i > 0) {
            double temp = vertexdata[i-1].bulge;
            vertexdata[i-1].bulge = vertexdata[i].bulge;
            vertexdata[i].bulge = temp;
        }
    }
}

MyData MyCreationDxf::CreateMyData(QList<DL_VertexData> list_vertexdata, std::string layer)
{
    MyData mydata;
    my_segment m_segment;

    QPointF start;
    QPointF end;
    double bulge;

    int prior;
    int next;

    for(prior=0;prior<list_vertexdata.size() - 1;prior++)
    {
        next = prior + 1;
        start = QPointF(list_vertexdata[prior].x,list_vertexdata[prior].y);
        end = QPointF(list_vertexdata[next].x,list_vertexdata[next].y);
        if(list_vertexdata[prior].bulge == 0)   //线段
        {
            if(qFabs(my_math_computer_length_twoPoint(start,end)) < 1e-10)   //跳过长度为0的线段
                continue;

            m_segment.set_m_line(start,end);
            m_segment.set_m_type(0);
        }
        else                                    //圆弧
        {
            bulge = list_vertexdata[prior].bulge;
            m_segment.set_m_arc(start,end,bulge);
            m_segment.set_m_type(1);
        }

        mydata.m_list_segment.append(m_segment);
    }

    mydata.m_type = 0;  //my_segment

    mydata.layer = layer;   //设置图层

    return mydata;
}

MyData MyCreationDxf::CreateMyData(CircleData cirdata)
{
    MyData mydata;

    mydata.m_cirlceData = cirdata;
    mydata.m_type = 1;  //Circle

    mydata.layer = cirdata.layer;   //设置图层

    return mydata;
}

MylistData MyCreationDxf::CreateMyListData()
{
    MyData mydata;
    MylistData mylistdata;

    QList<PolylineData> list_polylinedata = this->getDxfData().polyline_data;
    QList<CircleData> list_circledata = this->getDxfData().circle_data;

    //传入polylinedata
    for(int i=0;i<list_polylinedata.size();i++)
    {
        mydata = this->CreateMyData(list_polylinedata[i].vertexdata, list_polylinedata[i].layer);
        mylistdata.m_list_origin_data.append(mydata);
    }

    //传入circledata
    for(int i=0;i<list_circledata.size();i++)
    {
        mydata = this->CreateMyData(list_circledata[i]);
        mylistdata.m_list_origin_data.append(mydata);
    }

    return mylistdata;
}

MylistData MyCreationDxf::run()
{   

    this->TransformLineAndArcToPolyline();

    this->ProcessPolyline();

    QList<PolylineData>& pdata = this->getDxfData().polyline_data;

    for(int i=0;i<pdata.size();i++)
    {
        QList<DL_VertexData>& vdata = pdata[i].vertexdata;
        this->TransformCCW(vdata);    //逆时针存储
    }

    MylistData retval = this->CreateMyListData();

    return retval;

}

void MyCreationDxf::OutputDxf(const QString& filepath)
{
    DL_WriterA* dw = dxf.out(filepath.toStdString().c_str(), DL_Codes::AC1015);

    // section header:
    dxf.writeHeader(*dw);
    dw->sectionEnd();

    // section tables:
    dw->sectionTables();

    // VPORT:
    dxf.writeVPort(*dw);

    // LTYPE:
    dw->tableLinetypes(1);
    dxf.writeLinetype(*dw, DL_LinetypeData("CONTINUOUS", "Continuous", 0, 0, 0.0));
    dxf.writeLinetype(*dw, DL_LinetypeData("BYLAYER", "", 0, 0, 0.0));
    dxf.writeLinetype(*dw, DL_LinetypeData("BYBLOCK", "", 0, 0, 0.0));
    dw->tableEnd();

    //设置图层信息，保证图层唯一性和隔离
    // LAYER:   
    const auto& attr_set = LayerMgr::GetInstance().GetLayers();

    dw->tableLayers(attr_set.size());

    for(const auto& it : attr_set)
    {
        dxf.writeLayer(
            *dw,
            it,
            DL_Attributes(it.name, 1, 0x00ff0000, 15, "CONTINUOUS")
            );
    }

    dw->tableEnd();

    // STYLE:
    dw->tableStyle(1);
    DL_StyleData style("Standard", 0, 0.0, 1.0, 0.0, 0, 2.5, "txt", "");
    style.bold = false;
    style.italic = false;
    dxf.writeStyle(*dw, style);
    dw->tableEnd();

    // VIEW:
    dxf.writeView(*dw);

    // UCS:
    dxf.writeUcs(*dw);

    // APPID:
    dw->tableAppid(1);
    dxf.writeAppid(*dw, "ACAD");
    dw->tableEnd();

    // DIMSTYLE:
    dxf.writeDimStyle(*dw, 2.5, 0.625, 0.625, 0.625, 2.5);

    // BLOCK_RECORD:
    dxf.writeBlockRecord(*dw);
    dw->tableEnd();

    dw->sectionEnd();

    // BLOCK:
    dw->sectionBlocks();
    dxf.writeBlock(*dw, DL_BlockData("*Model_Space", 0, 0.0, 0.0, 0.0));
    dxf.writeEndBlock(*dw, "*Model_Space");
    dxf.writeBlock(*dw, DL_BlockData("*Paper_Space", 0, 0.0, 0.0, 0.0));
    dxf.writeEndBlock(*dw, "*Paper_Space");
    dxf.writeBlock(*dw, DL_BlockData("*Paper_Space0", 0, 0.0, 0.0, 0.0));
    dxf.writeEndBlock(*dw, "*Paper_Space0");
    dw->sectionEnd();

    //=================================================================================================
    //这里开始绘图
    // ENTITIES:
    dw->sectionEntities();

    DL_Attributes attr("MY_LAYER", -1, -1, -1, "BYLAYER");
    DL_ArcData adata(arc.start.x(), arc.start.y(), 0, arc.radius, arc.startAngle, arc.endAngle);
    dxf.writeArc(*dw, adata, attr);

    DL_LineData ldata(line.start.x(), line.start.y(), 0, line.end.x(), line.end.y(),0);
    dxf.writeLine(*dw, ldata, hole_attr);
    
    DL_CircleData cdata(da.m_cirlceData.cx, da.m_cirlceData.cy, 0, da.m_cirlceData.radius);
    dxf.writeCircle(*dw, cdata, attr);

    //=================================================================================================

    // end section ENTITIES:
    dw->sectionEnd();
    dxf.writeObjects(*dw, "MY_OBJECTS");
    dxf.writeObjectsEnd(*dw);

    dw->dxfEOF();
    dw->close();
    delete dw;

}