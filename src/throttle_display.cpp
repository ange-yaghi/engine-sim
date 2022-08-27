#include "../include/throttle_display.h"

#include "../include/geometry_generator.h"
#include "../include/engine_sim_application.h"

#include "../include/ui_utilities.h"

ThrottleDisplay::ThrottleDisplay() {
    m_engine = nullptr;
}

ThrottleDisplay::~ThrottleDisplay() {
    /* void */
}

void ThrottleDisplay::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);
}

void ThrottleDisplay::destroy() {
    UiElement::destroy();
}

void ThrottleDisplay::update(float dt) {
    UiElement::update(dt);
}

void ThrottleDisplay::render() {
    UiElement::render();

    drawFrame(m_bounds, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());

    const Bounds bounds = m_bounds.inset(10.0f);
    const Bounds title = bounds.verticalSplit(1.0f, 0.9f);
    drawCenteredText("THROTTLE", title.inset(10.0f), 24.0f);

    const Bounds mainDrawArea = bounds.verticalSplit(0.05f, 0.9f);
    renderThrottle(mainDrawArea);

    const Bounds speedControlDrawArea = bounds.verticalSplit(0.0f, 0.05f);
    renderSpeedControl(speedControlDrawArea);
}

void ThrottleDisplay::renderThrottle(const Bounds &bounds) {
    GeometryGenerator *gen = m_app->getGeometryGenerator();

    const float width = pixelsToUnits(bounds.width());
    const float height = pixelsToUnits(bounds.height());
    const float size = std::fmin(width, height);

    const Point origin = getRenderPoint(bounds.getPosition(Bounds::center));

    const float carbBore = size * 0.4f;
    const float carbHeight = size * 0.5f;
    const float plateWidth = carbBore * 0.8f;

    GeometryGenerator::Line2dParameters params;
    params.lineWidth = size * 0.01f;

    GeometryGenerator::Circle2dParameters circleParams;
    circleParams.radius = params.lineWidth / 2.0f;
    circleParams.maxEdgeLength = m_app->pixelsToUnits(5.0f);

    GeometryGenerator::GeometryIndices main, pivot, pivotShadow;
    gen->startShape();

    params.x0 = origin.x + carbBore / 2.0f;
    params.y0 = origin.y - carbHeight / 2.0f;
    params.x1 = origin.x + carbBore / 2.0f;
    params.y1 = origin.y + carbHeight / 2.0f;
    gen->generateLine2d(params);

    circleParams.center_x = params.x1;
    circleParams.center_y = params.y1;
    gen->generateCircle2d(circleParams);

    circleParams.center_x = params.x1;
    circleParams.center_y = params.y0;
    gen->generateCircle2d(circleParams);

    params.x0 = origin.x - carbBore / 2.0f;
    params.x1 = origin.x - carbBore / 2.0f;
    gen->generateLine2d(params);

    circleParams.center_x = params.x1;
    circleParams.center_y = params.y1;
    gen->generateCircle2d(circleParams);

    circleParams.center_x = params.x1;
    circleParams.center_y = params.y0;
    gen->generateCircle2d(circleParams);

    // Draw throttle plate
    const float throttleAngle = (float)m_engine->getThrottlePlateAngle();
    const float cos_theta = std::cosf(throttleAngle);
    const float sin_theta = std::sinf(throttleAngle);

    params.y0 = origin.y - sin_theta * plateWidth / 2.0f;
    params.x0 = origin.x - cos_theta * plateWidth / 2.0f;
    params.y1 = origin.y + sin_theta * plateWidth / 2.0f;
    params.x1 = origin.x + cos_theta * plateWidth / 2.0f;
    gen->generateLine2d(params);

    circleParams.center_x = params.x0;
    circleParams.center_y = params.y0;
    gen->generateCircle2d(circleParams);

    circleParams.center_x = params.x1;
    circleParams.center_y = params.y1;
    gen->generateCircle2d(circleParams);

    gen->endShape(&main);

    gen->startShape();
    circleParams.center_x = origin.x;
    circleParams.center_y = origin.y;
    circleParams.radius = size * 0.01f;
    gen->generateCircle2d(circleParams);
    gen->endShape(&pivot);

    gen->startShape();
    circleParams.center_x = origin.x;
    circleParams.center_y = origin.y;
    circleParams.radius = 2 * size * 0.01f;
    gen->generateCircle2d(circleParams);
    gen->endShape(&pivotShadow);

    m_app->getShaders()->SetBaseColor(m_app->getWhite());
    m_app->drawGenerated(main, 0x11, m_app->getShaders()->GetUiFlags());

    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(pivotShadow, 0x11, m_app->getShaders()->GetUiFlags());

    m_app->getShaders()->SetBaseColor(m_app->getWhite());
    m_app->drawGenerated(pivot, 0x11, m_app->getShaders()->GetUiFlags());
}

void ThrottleDisplay::renderSpeedControl(const Bounds &bounds) {
    GeometryGenerator *gen = m_app->getGeometryGenerator();

    Grid grid;
    grid.v_cells = 1;
    grid.h_cells = 3;
    const Bounds b = grid.get(bounds, 1, 0);

    const Point lm = b.getPosition(Bounds::lm);
    const Point rm = b.getPosition(Bounds::rm);

    const float s = static_cast<float>(m_engine->getSpeedControl());
    const Bounds bar = Bounds(b.width(), 2.0f, lm, Bounds::lm);
    const Bounds speedControlBar = Bounds(b.width() * s, 2.0f, lm, Bounds::lm);

    drawFrame(
        bar,
        1.0f,
        mix(m_app->getBackgroundColor(), m_app->getWhite(), 0.001f),
        mix(m_app->getBackgroundColor(), m_app->getRed(), 0.01f)
    );

    drawBox(
        speedControlBar,
        m_app->getRed()
    );
}
