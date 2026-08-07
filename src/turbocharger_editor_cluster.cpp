#include "../include/turbocharger_editor_cluster.h"

#include "../include/engine_sim_application.h"
#include "../include/units.h"

#include <sstream>
#include <iomanip>

namespace {
    struct FieldSpec {
        const char *label;
        const char *unit;
        double toDisplay;
        double displayMin, displayMax;
        int precision;
        double Turbocharger::Parameters::*member;
    };

    const FieldSpec kFields[] = {
        { "Turbine Flow Rate", "", 1.0, 0.0, 0.01, 5,
            &Turbocharger::Parameters::turbineRateK },
        { "Turbine Inertia", "", 1.0, 0.0, 0.0002, 6,
            &Turbocharger::Parameters::turbineInertia },
        { "Turbine Efficiency", "%", 100.0, 0.0, 100.0, 0,
            &Turbocharger::Parameters::turbineEfficiency },
        { "Compressor Efficiency", "%", 100.0, 0.0, 100.0, 0,
            &Turbocharger::Parameters::compressorEfficiency },
        { "Max Boost", "psi", 1.0 / units::psi, 0.0, 30.0, 1,
            &Turbocharger::Parameters::maxBoost },
        { "Spool Speed", "rpm", 1.0 / units::rpm(1.0), 0.0, 250000.0, 0,
            &Turbocharger::Parameters::spoolSpeed },
        { "Spin Down Time", "s", 1.0, 0.05, 3.0, 2,
            &Turbocharger::Parameters::spinDownTime },
        { "Discharge Volume", "L", 1.0 / units::L, 0.1, 5.0, 2,
            &Turbocharger::Parameters::compressorDischargeVolume },
        { "Inducer Diameter", "mm", 1.0 / units::mm, 20.0, 100.0, 1,
            &Turbocharger::Parameters::turbineInducerDiameter },
        { "Exducer Diameter", "mm", 1.0 / units::mm, 15.0, 80.0, 1,
            &Turbocharger::Parameters::turbineExducerDiameter },
    };

    constexpr float TitleHeight = 34.0f;
}

TurbochargerEditorCluster::TurbochargerEditorCluster() {
    m_engine = nullptr;
    m_turbo = nullptr;
    m_antiLagButton = nullptr;
}

TurbochargerEditorCluster::~TurbochargerEditorCluster() {
    /* void */
}

void TurbochargerEditorCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    for (const FieldSpec &spec : kFields) {
        FieldRow row;
        row.label = spec.label;
        row.unit = spec.unit;
        row.toDisplay = spec.toDisplay;
        row.displayMin = spec.displayMin;
        row.displayMax = spec.displayMax;
        row.precision = spec.precision;
        row.member = spec.member;
        row.field = addElement<UiNumberField>();
        row.field->m_label = spec.label;
        row.field->m_unit = spec.unit;
        row.field->m_min = spec.displayMin;
        row.field->m_max = spec.displayMax;
        row.field->m_precision = spec.precision;

        m_rows.push_back(row);
    }

    m_antiLagButton = addElement<UiButton>(this);
    m_antiLagButton->m_text = "ANTI-LAG: OFF";
}

void TurbochargerEditorCluster::signal(UiElement *element, Event event) {
    if (event != Event::Clicked) return;

    if (element == m_antiLagButton && m_engine != nullptr) {
        m_engine->setAntiLagEnabled(!m_engine->getAntiLagEnabled());
    }
}

// Mirrors how every other top-level cluster in this app lays itself out
// (see InfoCluster::render(), PerformanceCluster::render()): children's
// m_bounds are recomputed as an absolute rectangle from the parent's own
// m_bounds every render() call rather than via setLocalPosition, since
// m_bounds -- not m_localPosition -- is what this app actually uses to
// position top-level clusters (assigned externally each frame in
// EngineSimApplication::renderScene()).
void TurbochargerEditorCluster::layoutRows() {
    if (m_rows.empty()) return;

    const Bounds content = m_bounds.inset(4.0f);
    const Bounds listArea(
        content.left(), content.right(),
        content.bottom(), content.top() - TitleHeight);

    Grid grid;
    grid.h_cells = 1;
    grid.v_cells = (int)m_rows.size() + 1; // +1 for the anti-lag toggle row
    m_antiLagButton->m_bounds = grid.get(listArea, 0, 0).inset(1.5f);
    for (size_t i = 0; i < m_rows.size(); ++i) {
        m_rows[i].field->m_bounds = grid.get(listArea, 0, (int)i + 1).inset(1.5f);
    }
}

void TurbochargerEditorCluster::setEngine(Engine *engine) {
    m_engine = engine;
    m_turbo = (engine != nullptr && engine->getTurbochargerCount() > 0)
        ? engine->getTurbocharger(0)
        : nullptr;

    if (m_turbo == nullptr) return;

    // See Turbocharger::recoverSpecParameters() -- without this, re-calling
    // initialize() from Apply below would compound the wheel-size scale
    // factor onto turbineRateK/turbineInertia on every edit.
    m_params = m_turbo->recoverSpecParameters();

    for (FieldRow &row : m_rows) {
        row.field->m_value = (m_params.*row.member) * row.toDisplay;
    }
}

void TurbochargerEditorCluster::update(float dt) {
    UiElement::update(dt);

    if (m_engine != nullptr) {
        m_antiLagButton->m_text = m_engine->getAntiLagEnabled()
            ? "ANTI-LAG: ON" : "ANTI-LAG: OFF";
    }

    if (m_turbo == nullptr) return;

    bool changed = false;
    for (FieldRow &row : m_rows) {
        if (row.field->consumeChanged()) {
            m_params.*row.member = row.field->m_value / row.toDisplay;
            changed = true;
        }
    }

    if (changed) {
        // Cheap and self-contained -- see the class comment in the header
        // for why this hot-patch (rather than a full script
        // recompile/reload) is safe for this node.
        m_turbo->initialize(m_params);
    }
}

void TurbochargerEditorCluster::render() {
    drawPanel(m_bounds);
    drawText("TURBOCHARGER", m_bounds.inset(6.0f), 14.0f, Bounds::tl);

    if (m_turbo == nullptr) {
        drawText(
            "No turbocharger in this engine",
            m_bounds.inset(6.0f),
            12.0f,
            Bounds::tl + Point(0.0f, -0.06f));
        return;
    }

    std::stringstream readout;
    readout << std::fixed << std::setprecision(1)
        << "Boost: " << (m_turbo->getBoostPressure() / units::psi) << " psi   "
        << "Shaft: " << (int)units::toRpm(m_turbo->getShaftSpeed()) << " rpm";
    // Bounds::tr is (1, 0) (bottom-right) rather than top-right in this
    // codebase's convention -- using an explicit point instead to avoid it.
    drawAlignedText(readout.str(), m_bounds.inset(6.0f), 11.0f, Point(1.0f, 1.0f), Point(1.0f, 1.0f));

    layoutRows();

    UiElement::render();
}
