#ifndef ATG_ENGINE_SIM_CYLINDER_HEAD_H
#define ATG_ENGINE_SIM_CYLINDER_HEAD_H

#include "part.h"

#include "function.h"
#include "camshaft.h"
#include "exhaust_system.h"
#include "intake.h"

class CylinderBank;
class CylinderHead : public Part {
    public:
        struct Parameters {
            CylinderBank *Bank;
            Camshaft *ExhaustCam;
            Camshaft *IntakeCam;

            Function *ExhaustPortFlow;
            Function *IntakePortFlow;

            double CombustionChamberVolume;
            bool FlipDisplay = false;
        };

    public:
        CylinderHead();
        virtual ~CylinderHead();

        void initialize(const Parameters &params);
        virtual void destroy();

        double intakeFlowRate(int cylinder) const;
        double exhaustFlowRate(int cylinder) const;
        double intakeValveLift(int cylinder) const;
        double exhaustValveLift(int cylinder) const;

        inline ExhaustSystem *getExhaustSystem(int cylinderIndex) const { return m_exhaustSystems[cylinderIndex]; }
        void setAllExhaustSystems(ExhaustSystem *system);
        void setExhaustSystem(int i, ExhaustSystem *system);

        inline Intake *getIntake(int cylinderIndex) const { return m_intakes[cylinderIndex]; }
        void setAllIntakes(Intake *intake);
        void setIntake(int i, Intake *intake);

        inline bool getFlipDisplay() const { return m_flipDisplay; }
        inline double getCombustionChamberVolume() const { return m_combustionChamberVolume; }
        inline CylinderBank *getCylinderBank() const { return m_bank; }

    protected:
        ExhaustSystem **m_exhaustSystems;
        Intake **m_intakes;

        CylinderBank *m_bank;
        Camshaft *m_exhaustCamshaft;
        Camshaft *m_intakeCamshaft;

        Function *m_exhaustPortFlow;
        Function *m_intakePortFlow;

        double m_combustionChamberVolume;
        bool m_flipDisplay;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_HEAD_H */
