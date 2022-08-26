#ifndef ATG_ENGINE_SIM_CYLINDER_HEAD_H
#define ATG_ENGINE_SIM_CYLINDER_HEAD_H

#include "part.h"

#include "function.h"
#include "camshaft.h"
#include "exhaust_system.h"
#include "intake.h"

class Valvetrain;
class CylinderBank;
class CylinderHead : public Part {
    public:
        struct Parameters {
            CylinderBank *Bank;

            Function *ExhaustPortFlow;
            Function *IntakePortFlow;

            Valvetrain *Valvetrain;

            double CombustionChamberVolume;

            double IntakeRunnerVolume;
            double IntakeRunnerCrossSectionArea;
            double ExhaustRunnerVolume;
            double ExhaustRunnerCrossSectionArea;

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

        inline double getSoundAttenuation(int cylinderIndex) const { return m_soundAttenuation[cylinderIndex]; }
        void setSoundAttenuation(int i, double soundAttenuation);

        inline Intake *getIntake(int cylinderIndex) const { return m_intakes[cylinderIndex]; }
        void setAllIntakes(Intake *intake);
        void setIntake(int i, Intake *intake);

        inline bool getFlipDisplay() const { return m_flipDisplay; }
        inline double getCombustionChamberVolume() const { return m_combustionChamberVolume; }
        inline CylinderBank *getCylinderBank() const { return m_bank; }

        double getIntakeRunnerVolume() const { return m_intakeRunnerVolume; }
        double getIntakeRunnerCrossSectionArea() const { return m_intakeRunnerCrossSectionArea; }
        double getExhaustRunnerVolume() const { return m_exhaustRunnerVolume; }
        double getExhaustRunnerCrossSectionArea() const { return m_exhaustRunnerCrossSectionArea; }

        Camshaft *getExhaustCamshaft();
        Camshaft *getIntakeCamshaft();

    protected:
        ExhaustSystem **m_exhaustSystems;
        Intake **m_intakes;
        double *m_soundAttenuation;

        CylinderBank *m_bank;
        Valvetrain *m_valvetrain;

        Function *m_exhaustPortFlow;
        Function *m_intakePortFlow;

        double m_intakeRunnerVolume;
        double m_intakeRunnerCrossSectionArea;
        double m_exhaustRunnerVolume;
        double m_exhaustRunnerCrossSectionArea;

        double m_combustionChamberVolume;
        bool m_flipDisplay;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_HEAD_H */
