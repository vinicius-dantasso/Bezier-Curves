#ifndef _CURVES_H

#include "DXUT.h"
#include <cmath>
#include <cstring>
#include <fstream>
using namespace std;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

class Curves : public App
{
private:
    ID3D12RootSignature* rootSignature;
    ID3D12PipelineState* pipelineState;

    Mesh* ctrlPoint1;
    Mesh* ctrlPoint2;

    Mesh* curve;
    Mesh* finalCurve;

    Mesh* squarePoint1;
    Mesh* squarePoint2;
    Mesh* squarePoint3;
    Mesh* squarePoint4;

    static const uint MaxCtrl = 3;
    static const uint MaxCurve = 1000;
    static const uint MaxSquareVertex = 5;

    Vertex ctrl1[MaxCtrl];
    Vertex ctrl2[MaxCtrl];

    Vertex curvePoints[MaxCurve];
    Vertex showCurves[MaxCurve];

    Vertex back1[MaxSquareVertex];
    Vertex back2[MaxSquareVertex];
    Vertex back3[MaxSquareVertex];
    Vertex back4[MaxSquareVertex];

    uint ctrlCount1 = 0;
    uint ctrlCount2 = 0;
    uint index = 0;
    uint clickCount = 0;
    uint curveCount = 0;
    uint curveCount2 = 0;
    uint curveIndex = 0;
    uint totalCurves = 0;

    bool newCurve = false;
    bool createCurve = false;
    bool canDraw = false;
    bool fix = false;
    bool erase = false;
    bool saveCurve = false;
    bool loadCurve = false;

    float cx;
    float cy;
    float mx;
    float my;

public:
    void Init();
    void Update();
    void Display();
    void Finalize();

    void CreateVertices();
    void DrawVertices();

    void CreateCurve();
    void SaveCurve();
    void LoadCurve();
    void DeleteCurve();
    void DrawCurve();

    void DrawSquares();

    void BuildRootSignature();
    void BuildPipelineState();
};

#endif