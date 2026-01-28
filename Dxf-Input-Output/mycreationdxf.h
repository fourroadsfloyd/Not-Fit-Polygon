#ifndef MYCREATIONDXF_H
#define MYCREATIONDXF_H

#include<QList>
#include<QVector>
#include<QPointF>
#include<QLineF>
#include<QRectF>
#include <QtMath>
#include "my_dxf.h"


/***************************************************************************
*author:ysk
*date:2025-6-10
*description:
* *@brief 将DXF文件中的几何元素转换为MyData对象并存储
 * @param list_vertexdata 从DXF读取的顶点数据列表
 * @return 包含所有转换结果的MyData容器对象
 * @note 转换规则：
 *       1. 圆弧和线段转为多段线(polyline)
 *       2. 圆保持原样
 *       3. 每个独立图形封装为MyData对象
 *       4. 结果存储在返回对象的m_list_origin_data链表中
*****************************************************************************/
class MyCreationDxf: public DL_CreationAdapter
{
public:
    MyCreationDxf(){}

    virtual void addLayer(const DL_LayerData& data);
    virtual void addLine(const DL_LineData& data);
    virtual void addArc(const DL_ArcData& data);
    virtual void addCircle(const DL_CircleData& data); 
    virtual void addPolyline(const DL_PolylineData& data);
    virtual void addVertex(const DL_VertexData& data);

    DxfData& getDxfData();

    void TransformLineAndArcToPolyline();   //提取line、arc或line和arc组成的封闭图形，顶点按照某个顺序排列并将图形转化为polyline格式存储
    void processLinesAndArcs();
    void processArcsAndLines();
    void processOnlyLine();
    void processOnlyArc();
    bool findNextLineLink(QList<LineData>& list_line,QPointF& linkPoint,QList<DL_VertexData>& list_vertex,QPointF& startPoint,bool& hadClosed);
    bool findNextArcLink(QList<ArcData>& list_arc,QPointF& linkPoint,QList<DL_VertexData>& list_vertex,QPointF& startPoint,bool& hadClosed);
    DL_VertexData update_vertexdata(QPointF linkPoint,double bulge);

    void ProcessPolyline(); //1 让封闭图形顶点链表中的末点等于起点。2 让封闭图形的flags==1(图形闭合的标志位)

    double computer_Area(QList<DL_VertexData> vertexdata);

    void TransformCCW(QList<DL_VertexData>& vertexdata); //将顺时针的顶点顺序转换为逆时针

    //将每个封闭图形封装为MyData,其中Polyline传入my_segment,Circle传入DL_CircleData
    MyData CreateMyData(QList<DL_VertexData> list_vertexdata);

    MyData CreateMyData(DL_CircleData cirdata);

    MylistData CreateMyListData();

    MylistData run();

    void OutputDxf(const QString& filepath);

private:
    DxfData dxf_data;
};

#endif // MYCREATIONDXF_H
