#ifndef ATG_ENGINE_SIM_GEOMETRY_GENERATOR_H
#define ATG_ENGINE_SIM_GEOMETRY_GENERATOR_H

#include "delta.h"

class GeometryGenerator {
    public:
        struct GeometryIndices {
            int BaseIndex;
            int BaseVertex;
            int FaceCount;

            dbasic::Vertex *VertexData;

            bool Failed;
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

    public:
        GeometryGenerator();
        ~GeometryGenerator();

        void initialize(int vertexBufferSize, int indexBufferSize);
        void destroy();

        const dbasic::Vertex *getVertexData() const { return m_vertexData; }
        const unsigned short *getIndexData() const { return m_indexData; }

        int getCurrentVertexCount() const { return m_vertexPointer; }
        int getCurrentIndexCount() const { return m_indexPointer; }

        void reset();

        void generateFilledCircle(
            GeometryIndices *indices,
            const ysVector &normal,
            const ysVector &center,
            float radius,
            float maxEdgeLength
        );

        void generateFilledFanPolygon(
            GeometryIndices *indices,
            const ysVector &normal,
            const ysVector &up,
            const ysVector &center,
            float radius,
            float rotation,
            int segmentCount
        );

        void generateLineRing(
            GeometryIndices *indices,
            const LineRingParameters &params
        );

        void generateLineRingBalanced(
            GeometryIndices *indices,
            const LineRingParameters &params
        );

        void generateLine(
            GeometryIndices *indices,
            const LineParameters &params
        );

    protected:
        dbasic::Vertex *writeVertex();
        void writeFace(unsigned short i0, unsigned short i1, unsigned short i2);

    protected:
        static ysVector findOrthogonal(const ysVector &v);

    protected:
        dbasic::Vertex *m_vertexData;
        unsigned short *m_indexData;

        int m_vertexBufferSize;
        int m_indexBufferSize;

        int m_vertexPointer;
        int m_indexPointer;
};

#endif /* ATG_ENGINE_SIM_GEOMETRY_GENERATOR_H */
