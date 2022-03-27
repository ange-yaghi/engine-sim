#include "../include/crankshaft_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"

CrankshaftObject::CrankshaftObject() {
    m_crankshaft = nullptr;
}

CrankshaftObject::~CrankshaftObject() {
    /* void */
}

void CrankshaftObject::generateGeometry() {
    /* void */
}

void CrankshaftObject::render(const ViewParameters *view) {
    const int journalCount = m_crankshaft->m_rodJournalCount;

    for (int i = 0; i < journalCount; ++i) {
        const int layer = i;
        if (layer > view->Layer1 || layer < view->Layer0) return;

        const ysVector col = tintByLayer(ysColor::srgbiToLinear(0xAAAAAA), i);

        resetShader();
        setTransform(
            &m_crankshaft->m_body,
            m_crankshaft->m_throw,
            0.0f,
            0.0f,
            m_crankshaft->m_rodJournalAngles[i]);

        m_app->getShaders()->SetBaseColor(col);
        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Crankshaft"),
            0x10 - layer);
    }
}

void CrankshaftObject::process(float dt) {
    /* void */
}

void CrankshaftObject::destroy() {
    /* void */
}
