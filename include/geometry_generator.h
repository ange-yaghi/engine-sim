#ifndef ATG_ENGINE_SIM_GEOMETRY_GENERATOR_H
#define ATG_ENGINE_SIM_GEOMETRY_GENERATOR_H

#include "delta.h"

#include "../include/function.h"

#include "../include/ui_math.h"

class GeometryGenerator {
public:
    struct GeometryIndices {
        int BaseIndex = -1;
        int BaseVertex = -1;
        int FaceCount = -1;

        dbasic::Vertex *VertexData = nullptr;
    };

    struct LineRingParameters {
        ysVector normal;
        ysVector center;
        float radius;
        float patternHeight;
        float maxEdgeLength;
        float startAngle = 0.0f;
        float endAngle = ysMath::Constants::TWO_PI;
        float taperTail = 0.0f;
        float textureOffset = 0.0f;
        float textureWidthHeightRatio = 1.0f;
    };

    struct LineParameters {
        ysVector start;
        ysVector end;
        ysVector normal = ysMath::Constants::ZAxis;
        float patternHeight;
        float taperTail;
        float textureOffset = 0.0f;
        float textureWidthHeightRatio = 1.0f;
    };

    struct Line2dParameters {
        float x0, y0;
        float x1, y1;
        float lineWidth;
    };

    struct Ring2dParameters {
        float center_x = 0.0f, center_y = 0.0f;
        float startAngle = 0.0f, endAngle = ysMath::Constants::TWO_PI;
        float innerRadius = 0.5f, outerRadius = 1.0f;
        float maxEdgeLength = 2.0f;

        bool drawArrow = false;
        bool arrowOnEnd = true;
        float arrowLength = 0.0f;
    };

    struct Circle2dParameters {
        float center_x = 0.0f, center_y = 0.0f;
        float radius = 1.0f;
        float maxEdgeLength = 0.1f;
        float smallestAngle = ysMath::Constants::PI * 0.95f;
    };

    struct Cam2dParameters {
        float center_x = 0.0f, center_y = 0.0f;
        float baseRadius = 1.0f;
        float rollerRadius = 1.0f;
        Function *lift = nullptr;
        float maxEdgeLength = 0.1f;
        float smallestAngle = ysMath::Constants::PI * 0.95f;
    };

    struct Rhombus2dParameters {
        float center_x = 0.0f, center_y = 0.0f;
        float height = 0;
        float shear = 0;
        float width = 0;
    };

    struct Trapezoid2dParameters {
        float center_x = 0, center_y = 0;
        float height = 0;
        float base = 0;
        float top = 0;
    };

    struct FrameParameters {
        float x, y;
        float frameWidth, frameHeight;
        float lineWidth;
    };

    struct GridParameters {
        float x, y;
        float width, height;
        float div_x, div_y;
        float lineWidth;
    };

    struct PathParameters {
        Point *p0;
        Point *p1;
        int n0;
        int n1;

        int i = 0;
        float width;

        int v0 = -1;
        int v1 = -1;
        float pdir_x, pdir_y;
        float perp_x, perp_y;
    };

public:
    GeometryGenerator();
    ~GeometryGenerator();

    void initialize(int vertexBufferSize, int indexBufferSize);
    void destroy();

    const dbasic::Vertex *getVertexData() const { return m_vertexData; }
    const unsigned short *getIndexData() const { return m_indexData; }

    int getCurrentVertexCount() const { return m_state.vertexPointer; }
    int getCurrentIndexCount() const { return m_state.indexPointer; }

    void reset();

    bool generateFilledCircle(
        const ysVector &normal,
        const ysVector &center,
        float radius,
        float maxEdgeLength
    );

    bool generateFilledFanPolygon(
        const ysVector &normal,
        const ysVector &up,
        const ysVector &center,
        float radius,
        float rotation,
        int segmentCount
    );

    bool generateLineRing(
        const LineRingParameters &params);

    bool generateLineRingBalanced(
        const LineRingParameters &params);

    bool generateLine(
        const LineParameters &params);

    bool generateLine2d(
        const Line2dParameters &params);

    bool generateRing2d(
        const Ring2dParameters &params);

    bool generateFrame(
        const FrameParameters &params);

    bool generateGrid(
        const GridParameters &params);

    bool generateCircle2d(
        const Circle2dParameters &params);

    bool generateCam(
        const Cam2dParameters &params);

    bool generateRhombus(
        const Rhombus2dParameters &params);

    bool generateTrapezoid2d(
        const Trapezoid2dParameters &params);

    bool generateIsoscelesTriangle(
        float x, float y, float width, float height);

    bool startPath(PathParameters &params);
    bool generatePathSegment(PathParameters &params, bool detached = false);

    void startShape();
    void endShape(GeometryIndices *indices);

protected:
    void startSubshape();

    dbasic::Vertex *writeVertex();
    void writeFace(unsigned short i0, unsigned short i1, unsigned short i2);

    bool checkCapacity(int vertexCount, int indexCount);

protected:
    static ysVector findOrthogonal(const ysVector &v);

protected:
    dbasic::Vertex *m_vertexData;
    unsigned short *m_indexData;

    int m_vertexBufferSize;
    int m_indexBufferSize;

    struct State {
        int vertexPointer = 0;
        int indexPointer = 0;
        GeometryIndices currentShape;
        int subshapeVertexPointer = 0;
    } m_state;
};

#endif /* ATG_ENGINE_SIM_GEOMETRY_GENERATOR_H */
