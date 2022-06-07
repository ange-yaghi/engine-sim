#ifndef ATG_ENGINE_SIM_OSCILLOSCOPE_CLUSTER_H
#define ATG_ENGINE_SIM_OSCILLOSCOPE_CLUSTER_H

#include "ui_element.h"

#include "oscilloscope.h"
#include "engine_simulator.h"

class OscilloscopeCluster : public UiElement {
    public:
        OscilloscopeCluster();
        virtual ~OscilloscopeCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();
        virtual void signal(UiElement *element, Event event);

        virtual void update(float dt);
        virtual void render();

        EngineSimulator *m_simulator;

        Oscilloscope *getExhaustFlowOscilloscope() { return m_exhaustFlowScope; }
        Oscilloscope *getAudioWaveformOscilloscope() { return m_audioWaveformScope; }

    protected:
        void renderScope(
            Oscilloscope *osc,
            const Bounds &bounds,
            const std::string &title,
            bool overlay=false);

        Oscilloscope
            *m_torqueScope,
            *m_hpScope,
            *m_exhaustFlowScope,
            *m_audioWaveformScope,
            *m_currentFocusScope0,
            *m_currentFocusScope1;
        float m_updatePeriod;
        float m_updateTimer;

        double m_torque;
        double m_hp;
};

#endif /* ATG_ENGINE_SIM_OSCILLOSCOPE_CLUSTER_H */
