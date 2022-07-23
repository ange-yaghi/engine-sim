#ifndef ATG_ENGINE_SIM_OSCILLOSCOPE_CLUSTER_H
#define ATG_ENGINE_SIM_OSCILLOSCOPE_CLUSTER_H

#include "ui_element.h"

#include "simulator.h"
#include "oscilloscope.h"

class OscilloscopeCluster : public UiElement {
    private:
        static constexpr int MaxLayeredScopes = 5;

    public:
        OscilloscopeCluster();
        virtual ~OscilloscopeCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();
        virtual void signal(UiElement *element, Event event);

        virtual void update(float dt);
        virtual void render();

        Simulator *m_simulator;

        Oscilloscope *getTotalExhaustFlowOscilloscope() const { return m_totalExhaustFlowScope; }
        Oscilloscope *getExhaustFlowOscilloscope() const { return m_exhaustFlowScope; }
        Oscilloscope *getIntakeFlowOscilloscope() const { return m_intakeFlowScope; }
        Oscilloscope *getAudioWaveformOscilloscope() const { return m_audioWaveformScope; }
        Oscilloscope *getIntakeValveLiftOscilloscope() const { return m_intakeValveLiftScope; }
        Oscilloscope *getExhaustValveLiftOscilloscope() const { return m_exhaustValveLiftScope; }
        Oscilloscope *getCylinderPressureScope() const { return m_cylinderPressureScope; }
        Oscilloscope *getSparkAdvanceScope() const { return m_sparkAdvanceScope; }
        Oscilloscope *getCylinderMoleculesScope() const { return m_cylinderMoleculesScope; }
        Oscilloscope *getPvScope() const { return m_pvScope; }

    protected:
        void renderScope(
            Oscilloscope *osc,
            const Bounds &bounds,
            const std::string &title,
            bool overlay=false);

        Oscilloscope
            *m_torqueScope,
            *m_hpScope,
            *m_audioWaveformScope,
            *m_exhaustValveLiftScope,
            *m_intakeValveLiftScope,
            *m_cylinderPressureScope,
            *m_totalExhaustFlowScope,
            *m_exhaustFlowScope,
            *m_intakeFlowScope,
            *m_cylinderMoleculesScope,
            *m_sparkAdvanceScope,
            *m_pvScope,
            *m_currentFocusScopes[MaxLayeredScopes];
        float m_updatePeriod;
        float m_updateTimer;

        double m_torque;
        double m_hp;
};

#endif /* ATG_ENGINE_SIM_OSCILLOSCOPE_CLUSTER_H */
