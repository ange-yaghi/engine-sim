#include "../include/geometry_generator.h"

GeometryGenerator::GeometryGenerator() {
    m_vertexData = nullptr;
    m_indexData = nullptr;

    m_vertexPointer = 0;
    m_indexPointer = 0;

    m_indexBufferSize = 0;
    m_vertexBufferSize = 0;
}

GeometryGenerator::~GeometryGenerator() {
    /* void */
}

void GeometryGenerator::initialize(int vertexBufferSize, int indexBufferSize) {
    m_vertexData = new dbasic::Vertex[vertexBufferSize];
    m_indexData = new unsigned short[indexBufferSize];

    m_vertexBufferSize = vertexBufferSize;
    m_indexBufferSize = indexBufferSize;
}

void GeometryGenerator::destroy() {
    delete[] m_vertexData;
    delete[] m_indexData;
}

void GeometryGenerator::reset() {
    m_vertexPointer = 0;
    m_indexPointer = 0;
}

void GeometryGenerator::generateFilledCircle(
    GeometryIndices *indices,
    const ysVector &normal,
    const ysVector &center,
    float radius,
    float maxEdgeLength)
{
    // edge_length = (sin(theta) * radius) * 2
    // theta = arcsin(edge_length / (2 * radius))

    const float angle = std::asinf(maxEdgeLength / (2 * radius));
    const float steps = ysMath::Constants::TWO_PI / angle;
    
    int wholeSteps = (int)std::ceilf(steps);
    wholeSteps = (wholeSteps < 3)
        ? 3
        : wholeSteps;

    generateFilledFanPolygon(
        indices,
        normal,
        findOrthogonal(normal),
        center,
        radius,
        0.0f,
        wholeSteps);
}

void GeometryGenerator::generateFilledFanPolygon(
    GeometryIndices *indices,
    const ysVector &normal,
    const ysVector &up,
    const ysVector &center,
    float radius,
    float rotation,
    int segmentCount)
{
    const int vertexCount = 1 + segmentCount;
    const int faceCount = segmentCount;
    const int indexCount = faceCount * 3;

    if (vertexCount + m_vertexPointer > m_vertexBufferSize ||
        indexCount + m_indexPointer > m_indexBufferSize)
    {
        indices->Failed = true;
        return;
    }

    indices->BaseIndex = m_indexPointer;
    indices->BaseVertex = m_vertexPointer;
    indices->FaceCount = faceCount;
    indices->VertexData = &m_vertexData[m_vertexPointer];
    indices->Failed = false;

    // Generate center vertex
    dbasic::Vertex *centerVertex = writeVertex();
    centerVertex->Normal = ysMath::GetVector4(normal);
    centerVertex->Pos = ysMath::GetVector4(center);
    centerVertex->TexCoord = ysVector2(0.5f, 0.5f);

    const float angleStep = ysMath::Constants::TWO_PI / segmentCount;

    const ysVector right = ysMath::Cross(up, normal);
    ysMatrix T = ysMath::LoadMatrix(
        right,
        up,
        normal,
        ysMath::ExtendVector(center)
    );
    T = ysMath::Transpose(T);

    for (int i = 0; i < segmentCount; ++i) {
        const float angle0 = angleStep * i + rotation;
        const float x0 = std::cosf(angle0);
        const float y0 = std::sinf(angle0);

        const ysVector pos = ysMath::LoadVector(x0 * radius, y0 * radius, 0.0f, 1.0f);

        dbasic::Vertex *newVertex = writeVertex();
        newVertex->Normal = normal;
        newVertex->Pos = ysMath::MatMult(T, pos);
        newVertex->TexCoord = ysVector2(0.5f * x0 + 0.5f, 0.5f * y0 + 0.5f);
    }

    for (int i = 0; i < segmentCount; ++i) {
        writeFace(0, i + 1, 1 + ((i + 1) % segmentCount));
    }
}

void GeometryGenerator::generateLineRing(
    GeometryIndices *indices,
    const ysVector &normal,
    const ysVector &center,
    float radius,
    float patternHeight,
    float maxEdgeLength,
    float startAngle,
    float endAngle,
    float taperTail) {
    // edge_length = (sin(theta) * radius) * 2
    // theta = arcsin(edge_length / (2 * radius))

    const float actualStartAngle = startAngle - taperTail;
    const float actualEndAngle = endAngle + taperTail;

    const float maxOuterRadius = radius + (patternHeight / 2);

    const float angle = std::asinf(maxEdgeLength / (2 * maxOuterRadius));
    const float steps = (actualEndAngle - actualStartAngle) / angle;

    int segmentCount = (int)std::ceilf(steps);
    segmentCount = (segmentCount < 3)
        ? 3
        : segmentCount;

    const int vertexCount = (segmentCount + 1) * 2;
    const int faceCount = segmentCount * 2;
    const int indexCount = faceCount * 3;

    const ysVector up = findOrthogonal(normal);

    if (vertexCount + m_vertexPointer > m_vertexBufferSize ||
        indexCount + m_indexPointer > m_indexBufferSize) {
        indices->Failed = true;
        return;
    }

    indices->BaseIndex = m_indexPointer;
    indices->BaseVertex = m_vertexPointer;
    indices->FaceCount = faceCount;
    indices->VertexData = &m_vertexData[m_vertexPointer];
    indices->Failed = false;

    // Generate center vertex
    const float angleStep = (actualEndAngle - actualStartAngle) / segmentCount;

    const ysVector right = ysMath::Cross(up, normal);
    ysMatrix T = ysMath::LoadMatrix(
        right,
        up,
        normal,
        ysMath::ExtendVector(center)
    );
    T = ysMath::Transpose(T);

    for (int i = 0; i <= segmentCount; ++i) {
        float angle0 = angleStep * i + actualStartAngle;
        const float x0 = std::cosf(angle0);
        const float y0 = std::sinf(angle0);

        if (angle0 >= actualEndAngle) angle0 = actualEndAngle;
        else if (angle0 <= actualStartAngle) angle0 = actualStartAngle;

        float taper = 1.0f;
        if (taperTail != 0) {
            if (angle0 >= actualStartAngle && angle0 < startAngle) {
                taper = (angle0 - actualStartAngle) / taperTail;
            }
            else if (angle0 > endAngle && angle0 <= actualEndAngle) {
                taper = 1.0f - (angle0 - endAngle) / taperTail;
            }
        }

        const float innerRadius = radius - (patternHeight / 2) * taper;
        const float outerRadius = radius + (patternHeight / 2) * taper;

        const ysVector outerPos = ysMath::LoadVector(x0 * outerRadius, y0 * outerRadius, 0.0f, 1.0f);
        const ysVector innerPos = ysMath::LoadVector(x0 * innerRadius, y0 * innerRadius, 0.0f, 1.0f);

        dbasic::Vertex *outerVertex = writeVertex();
        outerVertex->Normal = normal;
        outerVertex->Pos = ysMath::MatMult(T, outerPos);
        outerVertex->TexCoord = ysVector2(0.2f * angle0 * radius / patternHeight, 1.0f);

        dbasic::Vertex *innerVertex = writeVertex();
        innerVertex->Normal = normal;
        innerVertex->Pos = ysMath::MatMult(T, innerPos);
        innerVertex->TexCoord = ysVector2(0.2f * angle0 * radius / patternHeight, 0.0f);
    }

#define OUTER(i) (((i)) * 2)
#define INNER(i) (((i)) * 2 + 1)

    for (int i = 0; i < segmentCount; ++i) {
        writeFace(INNER(i), OUTER(i + 1), INNER(i + 1));
        writeFace(INNER(i), OUTER(i), OUTER(i + 1));
    }
}

dbasic::Vertex *GeometryGenerator::writeVertex() {
    return &m_vertexData[m_vertexPointer++];
}

void GeometryGenerator::writeFace(unsigned short i0, unsigned short i1, unsigned short i2) {
    m_indexData[m_indexPointer] = i0;
    m_indexData[m_indexPointer + 1] = i1;
    m_indexData[m_indexPointer + 2] = i2;

    m_indexPointer += 3;
}

ysVector GeometryGenerator::findOrthogonal(const ysVector &v) {
    ysVector base = ysMath::Constants::XAxis;
    const float s = ysMath::GetScalar(ysMath::Dot(v, base));
    if (abs(s) > 0.99f) {
        base = ysMath::Constants::YAxis;
    }

    return ysMath::Normalize(ysMath::Cross(v, base));
}
