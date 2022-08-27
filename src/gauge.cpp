#include "../include/gauge.h"

#include "../include/engine_sim_application.h"
#include "../include/constants.h"

Gauge::Gauge() {
    m_thetaMin = (float)constants::pi;
    m_thetaMax = 0.0f;

    m_min = m_max = 0;
    m_maxMinorTick = INT_MAX;
    m_value = 0;
    m_minorStep = 1;
    m_majorStep = 10;

    m_minorTickWidth = 1.0f;
    m_majorTickWidth = 2.0f;

    m_minorTickLength = 5.0f;
    m_majorTickLength = 10.0f;

    m_outerRadius = 0.0f;
    m_renderText = false;

    m_needleInnerRadius = 0.0f;
    m_needleOuterRadius = 0.0f;
    m_needleWidth = 1.0f;

    m_needlePosition = 0.0f;
    m_needleVelocity = 0.0f;
    m_needleMaxVelocity = 2.0f;
    m_needleKs = 1000.0f;
    m_needleKd = 25.0f;

    m_gamma = 1.0f;
}

Gauge::~Gauge() {
    /* void */
}

void Gauge::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);
}

void Gauge::destroy() {
    /* void */
}

void Gauge::update(float dt) {
    const float value = std::fmaxf((float)m_min, std::fmin((float)m_max, (float)m_value));
    const float needle_s = std::powf((value - m_min) / std::abs(m_max - m_min), m_gamma);
    const float F =
        m_needleKs * (needle_s - m_needlePosition)
        - m_needleKd * m_needleVelocity;

    m_needleVelocity = std::fminf(
            m_needleMaxVelocity,
            std::fmaxf(m_needleVelocity + F * dt, -m_needleMaxVelocity));
    m_needlePosition += m_needleVelocity * dt;
    m_needlePosition = std::fmax(0.0f, std::fmin(1.0f, m_needlePosition));
}

void Gauge::render() {
    GeometryGenerator *generator = m_app->getGeometryGenerator();

    const Point origin = getRenderPoint(m_bounds.getPosition(Bounds::center) + m_center);
    const float outerRadius = pixelsToUnits(m_outerRadius);
    const float minorTickWidth = pixelsToUnits(m_minorTickWidth);
    const float majorTickWidth = pixelsToUnits(m_majorTickWidth);
    const float minorTickLength = pixelsToUnits(m_minorTickLength);
    const float majorTickLength = pixelsToUnits(m_majorTickLength);

    GeometryGenerator::GeometryIndices ticks, needle;

    GeometryGenerator::Line2dParameters lineParams;
    generator->startShape();
    for (int i = 0; i <= std::abs(m_max - m_min); i += m_minorStep) {
        const float s = std::powf((float)i / std::abs(m_max - m_min), m_gamma);
        const float theta = s * m_thetaMax + (1 - s) * m_thetaMin;

        const float tickLength = (i % m_majorStep) == 0
            ? majorTickLength
            : minorTickLength;

        const float tickWidth = (i % m_majorStep) == 0
            ? majorTickWidth
            : minorTickWidth;

        const Point dir(std::cos(theta), std::sin(theta));
        const Point inner = dir * (outerRadius - tickLength) + origin;
        const Point outer = dir * outerRadius + origin;
        const Point text = dir * (outerRadius - majorTickLength * 2) + origin;

        lineParams.lineWidth = tickWidth;
        lineParams.x0 = inner.x;
        lineParams.x1 = outer.x;
        lineParams.y0 = inner.y;
        lineParams.y1 = outer.y;

        if ((i % m_majorStep) == 0 || (i + m_minorStep) <= m_maxMinorTick) {
            generator->generateLine2d(lineParams);
        }

        if ((i % m_majorStep) == 0 && m_renderText) {
            drawCenteredText(
                    "n",
                    Bounds(0.0f, 0.0f, unitsToPixels(text - origin) + m_bounds.getPosition(Bounds::center) + m_center, Bounds::center),
                    12);
        }
    }

    generator->endShape(&ticks);

    generator->startShape();

    const float needleAngle = m_needlePosition * m_thetaMax + (1 - m_needlePosition) * m_thetaMin;
    const Point needleDir(std::cos(needleAngle), std::sin(needleAngle));
    const Point needleOuter = needleDir * pixelsToUnits(m_needleOuterRadius) + origin;
    const Point needleInner = needleDir * pixelsToUnits(m_needleInnerRadius) + origin;

    lineParams.lineWidth = pixelsToUnits(m_needleWidth);
    lineParams.x0 = needleInner.x;
    lineParams.x1 = needleOuter.x;
    lineParams.y0 = needleInner.y;
    lineParams.y1 = needleOuter.y;
    generator->generateLine2d(lineParams);
    generator->endShape(&needle);

    resetShader();

    GeometryGenerator::Ring2dParameters ringParams;
    ringParams.arrowOnEnd = false;
    ringParams.drawArrow = false;
    ringParams.center_x = origin.x;
    ringParams.center_y = origin.y;
    ringParams.maxEdgeLength = pixelsToUnits(5);
    for (const Band &band : m_bands) {
        ringParams.outerRadius = outerRadius + pixelsToUnits(band.radial_offset);
        ringParams.innerRadius = outerRadius + pixelsToUnits(band.radial_offset - band.width);

        const float s0 = std::powf((float)(band.start - m_min) / std::abs(m_max - m_min), m_gamma);
        const float angle0 = s0 * m_thetaMax + (1 - s0) * m_thetaMin;

        const float s1 = std::powf((float)(band.end - m_min) / std::abs(m_max - m_min), m_gamma);
        const float angle1 = s1 * m_thetaMax + (1 - s1) * m_thetaMin;

        ringParams.startAngle = std::fminf(angle0, angle1) + band.shorten_end;
        ringParams.endAngle = std::fmaxf(angle0, angle1) - band.shorten_start;

        GeometryGenerator::GeometryIndices bandIndices;
        generator->startShape();
        generator->generateRing2d(ringParams);
        generator->endShape(&bandIndices);

        m_app->getShaders()->SetBaseColor(band.color);
        m_app->drawGenerated(bandIndices, 0x11, m_app->getShaders()->GetUiFlags());
    }

    m_app->getShaders()->SetBaseColor(m_app->getForegroundColor());
    m_app->drawGenerated(ticks, 0x11, m_app->getShaders()->GetUiFlags());

    m_app->getShaders()->SetBaseColor(m_app->getHightlight1Color());
    m_app->drawGenerated(needle, 0x11, m_app->getShaders()->GetUiFlags());
}
