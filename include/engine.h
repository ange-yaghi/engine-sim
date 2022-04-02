#ifndef ATG_ENGINE_SIM_ENGINE_H
#define ATG_ENGINE_SIM_ENGINE_H

#include "part.h"

#include "piston.h"
#include "connecting_rod.h"
#include "crankshaft.h"
#include "cylinder_bank.h"
#include "cylinder_head.h"
#include "exhaust_system.h"
#include "ignition_module.h"
#include "intake.h"

class Engine : public Part {
    public:
        struct Parameters {
            int CylinderBanks;
            int CylinderCount;
            int CrankshaftCount;
            int ExhaustSystemCount;
            int IntakeCount;
        };

    public:
        Engine();
        virtual ~Engine();

        void initialize(const Parameters &params);
        virtual void destroy();

        int getCylinderBankCount() const { return m_cylinderBankCount; }
        int getCylinderCount() const { return m_cylinderCount; }
        int getCrankshaftCount() const { return m_crankshaftCount; }
        int getExhaustSystemCount() const { return m_exhaustSystemCount; }
        int getIntakeCount() const { return m_intakeCount; }

        Crankshaft *getCrankshaft(int i) const { return &m_crankshafts[i]; }
        CylinderBank *getCylinderBank(int i) const { return &m_cylinderBanks[i]; }
        CylinderHead *getHead(int i) const { return &m_heads[i]; }
        Piston *getPiston(int i) const { return &m_pistons[i]; }
        ConnectingRod *getConnectingRod(int i) const { return &m_connectingRods[i]; }
        IgnitionModule *getIgnitionModule() { return &m_ignitionModule; }
        ExhaustSystem *getExhaustSystem(int i) const { return &m_exhaustSystems[i]; }
        Intake *getIntake(int i) const { return &m_intakes[i]; }

        double getRpm() const;

    protected:
        Crankshaft *m_crankshafts;
        int m_crankshaftCount;

        CylinderBank *m_cylinderBanks;
        CylinderHead *m_heads;
        int m_cylinderBankCount;

        Piston *m_pistons;
        ConnectingRod *m_connectingRods;
        int m_cylinderCount;

        ExhaustSystem *m_exhaustSystems;
        int m_exhaustSystemCount;

        Intake *m_intakes;
        int m_intakeCount;

        IgnitionModule m_ignitionModule;
};

#endif /* ATG_ENGINE_SIM_ENGINE_H */
