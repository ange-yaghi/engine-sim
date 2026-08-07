#include "../include/dyno_chart.h"

#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

#include <sstream>
#include <iomanip>

DynoChart::DynoChart() {
    m_precision = 0;
    m_lineColor = ysMath::Constants::One;

    m_xMax = 1000.0;
    m_yMax = 100.0;
    m_redlineRpm = 0.0;

    m_peakValue = 0.0;
    m_peakRpm = 0.0;

    m_scope = nullptr;
}

DynoChart::~DynoChart() {
    /* void */
}

void DynoChart::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_scope = addElement<Oscilloscope>();
    m_scope->setBufferSize(200);
    m_scope->m_lineWidth = 3.0;
    m_scope->m_drawZero = false;
    m_scope->m_drawReverse = false;
    m_scope->m_dynamicallyResizeX = false;
    m_scope->m_dynamicallyResizeY = false;
}

void DynoChart::destroy() {
    UiElement::destroy();
}

void DynoChart::update(float dt) {
    UiElement::update(dt);
}

void DynoChart::addDataPoint(double rpm, double value) {
    m_scope->addDataPoint(rpm, value);

    if (value > m_peakValue) {
        m_peakValue = value;
        m_peakRpm = rpm;
    }

    if (value * 1.15 > m_yMax) {
        m_yMax = value * 1.15;
    }
}

void DynoChart::reset() {
    m_scope->reset();
    m_peakValue = 0.0;
    m_peakRpm = 0.0;
    m_yMax = 100.0;
}

Point DynoChart::dataToLocal(double rpm, double value, const Bounds &plotArea) const {
    const double xMax = std::fmax(m_xMax, 1.0);
    const double yMax = std::fmax(m_yMax, 1.0);

    const float s_x = (float)(rpm / xMax);
    const float s_y = (float)(value / yMax);

    return plotArea.getPosition(Bounds::bl) + Point(s_x * plotArea.width(), s_y * plotArea.height());
}

void DynoChart::render() {
    drawPanel(m_bounds);

    const Bounds insetBounds = m_bounds.inset(15.0f);

    drawText(m_title, insetBounds, 18.0f, Bounds::tl);

    constexpr float axisLabelWidth = 55.0f;
    constexpr float axisLabelHeight = 22.0f;
    constexpr float topMargin = 28.0f;

    const Bounds plotArea(
        insetBounds.left() + axisLabelWidth,
        insetBounds.right(),
        insetBounds.bottom() + axisLabelHeight,
        insetBounds.top() - topMargin);

    GeometryGenerator *generator = m_app->getGeometryGenerator();
    GeometryGenerator::Line2dParameters lineParams;
    lineParams.lineWidth = pixelsToUnits(1.0f);

    constexpr int gridDivisions = 5;

    GeometryGenerator::GeometryIndices gridLines;
    generator->startShape();

    for (int i = 0; i <= gridDivisions; ++i) {
        const double value = m_yMax * i / gridDivisions;
        const Point a = getRenderPoint(dataToLocal(0.0, value, plotArea));
        const Point b = getRenderPoint(dataToLocal(m_xMax, value, plotArea));

        lineParams.x0 = a.x;
        lineParams.y0 = a.y;
        lineParams.x1 = b.x;
        lineParams.y1 = b.y;
        generator->generateLine2d(lineParams);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(m_precision) << value;
        drawAlignedText(
            ss.str(),
            Bounds(0.0f, 0.0f, dataToLocal(0.0, value, plotArea), Bounds::rm).move({ -6.0f, 0.0f }),
            13.0f, Bounds::rm, Bounds::rm);
    }

    for (int i = 0; i <= gridDivisions; ++i) {
        const double rpm = m_xMax * i / gridDivisions;
        const Point a = getRenderPoint(dataToLocal(rpm, 0.0, plotArea));
        const Point b = getRenderPoint(dataToLocal(rpm, m_yMax, plotArea));

        lineParams.x0 = a.x;
        lineParams.y0 = a.y;
        lineParams.x1 = b.x;
        lineParams.y1 = b.y;
        generator->generateLine2d(lineParams);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << rpm;
        drawCenteredText(
            ss.str(),
            Bounds(0.0f, 0.0f, dataToLocal(rpm, 0.0, plotArea), Bounds::tm).move({ 0.0f, -18.0f }),
            13.0f, Bounds::tm);
    }

    generator->endShape(&gridLines);

    resetShader();
    m_app->getShaders()->SetBaseColor(mix(m_app->getForegroundColor(), m_app->getBackgroundColor(), 0.82f));
    m_app->drawGenerated(gridLines, 0x11, m_app->getShaders()->GetUiFlags());

    if (m_redlineRpm > 0.0 && m_redlineRpm <= m_xMax) {
        const Point a = getRenderPoint(dataToLocal(m_redlineRpm, 0.0, plotArea));
        const Point b = getRenderPoint(dataToLocal(m_redlineRpm, m_yMax, plotArea));

        GeometryGenerator::Line2dParameters redlineParams;
        redlineParams.lineWidth = pixelsToUnits(2.0f);
        redlineParams.x0 = a.x;
        redlineParams.y0 = a.y;
        redlineParams.x1 = b.x;
        redlineParams.y1 = b.y;

        GeometryGenerator::GeometryIndices redline;
        generator->startShape();
        generator->generateLine2d(redlineParams);
        generator->endShape(&redline);

        resetShader();
        m_app->getShaders()->SetBaseColor(m_app->getRed());
        m_app->drawGenerated(redline, 0x11, m_app->getShaders()->GetUiFlags());
    }

    m_scope->m_xMin = 0.0;
    m_scope->m_xMax = m_xMax;
    m_scope->m_yMin = 0.0;
    m_scope->m_yMax = m_yMax;
    m_scope->i_color = m_lineColor;
    m_scope->m_bounds = plotArea;

    if (m_peakValue > 0.0) {
        const Point peakLocal = dataToLocal(m_peakRpm, m_peakValue, plotArea);
        drawBox(Bounds(8.0f, 8.0f, peakLocal, Bounds::center), m_lineColor);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(m_precision) << m_peakValue << " " << m_unit
           << " @ " << std::setprecision(0) << m_peakRpm << " rpm";
        drawAlignedText(
            ss.str(),
            Bounds(0.0f, 0.0f, peakLocal, Bounds::bm).move({ 0.0f, 12.0f }),
            14.0f, Bounds::bm, Bounds::bm);
    }

    UiElement::render();
}
