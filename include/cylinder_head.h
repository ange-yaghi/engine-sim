#ifndef ATG_ENGINE_SIM_CYLINDER_HEAD_H
#define ATG_ENGINE_SIM_CYLINDER_HEAD_H

#include "part.h"

#include "function.h"
#include "camshaft.h"

class CylinderBank;
class CylinderHead : public Part {
    public:
        struct Parameters {
            CylinderBank *Bank;
            Camshaft *ExhaustCam;
            Camshaft *IntakeCam;

            Function *ExhaustPortFlow;
            Function *IntakePortFlow;
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

        CylinderBank *m_bank;
        Camshaft *m_exhaustCamshaft;
        Camshaft *m_intakeCamshaft;

        Function *m_exhaustPortFlow;
        Function *m_intakePortFlow;

        double m_flow;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_HEAD_H */
