#ifndef ATG_ENGINE_SIM_COMBUSTION_CHAMBER_H
#define ATG_ENGINE_SIM_COMBUSTION_CHAMBER_H

#include "scs.h"

#include "piston.h"
#include "cylinder_bank.h"
#include "gas_system.h"
#include "cylinder_head.h"

class CombustionChamber : public atg_scs::ForceGenerator {
    public:
        struct FlameEvent {
            double lastVolume;
            double travel;
        };

    public:
        CombustionChamber();
        virtual ~CombustionChamber();

        void initialize(double p0, double t0);
        virtual void apply(atg_scs::SystemState *system);

        Piston *m_piston;
        CylinderBank *m_bank;
        CylinderHead *m_head;

        double volume() const;

        void ignite();
        void update(double dt);

        double m_blowbyK;
        double m_crankcasePressure;
        double m_manifoldPressure;

        GasSystem m_system;
        FlameEvent m_flameEvent;
        bool m_lit;
};

#endif /* ATG_ENGINE_SIM_COMBUSTION_CHAMBER_H */
