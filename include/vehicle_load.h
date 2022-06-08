#ifndef ATG_ENGINE_SIM_VEHICLE_LOAD_H
#define ATG_ENGINE_SIM_VEHICLE_LOAD_H

#include "scs.h"

#include "crankshaft.h"

class VehicleLoad : public atg_scs::ForceGenerator {
    public:
        VehicleLoad();
        virtual ~VehicleLoad();

        void initialize(Crankshaft *outputShaft, atg_scs::RigidBody *flywheel);

        virtual void apply(atg_scs::SystemState *system);

    protected:
        Crankshaft *m_outputShaft;
        atg_scs::RigidBody *m_flywheel;
};

#endif /* ATG_ENGINE_SIM_VEHICLE_LOAD_H */
