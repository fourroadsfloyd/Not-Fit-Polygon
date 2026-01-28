#ifndef MY_DXF_H
#define MY_DXF_H

#include <QList>
#include <QPointF>

//======================================================================
/*
 * author:ysk
 * date:2025-6-8
 * description:
 */
typedef struct LineData{
    double x1;
    double y1;
    double z1;

    double x2;
    double y2;
    double z2;

    QPointF startP;
    QPointF endP;

    int flag = 0; //用来判断线段的起点是否添加到Vertex中. 0：否 1：是

    std::string layer;

}LineData;

typedef struct ArcData{
    double cx;
    double cy;
    double cz;

    double radius;

    double angle1;

    double angle2;

    QPointF startP;
    QPointF endP;

    double bulge; //凸度

    int flag = 0; //判断是否读入vertex中. 0:否 1:是

    std::string layer;

}ArcData;


typedef struct PolylineData{
    unsigned int number;

    unsigned int m;

    unsigned int n;

    double elevation;

    int flags;

    QList<DL_VertexData> vertexdata;

    std::string layer;

}PolylineData;

typedef struct CircleData{
    double cx;

    double cy;

    double cz;

    double radius;

    std::string layer;
}CircleData;

typedef struct{

    QList<LineData> line_data;
    QList<ArcData> arc_data;
    QList<PolylineData> polyline_data;
    QList<CircleData> circle_data;

}DxfData;
//=================================================================

#endif // MY_DXF_H
