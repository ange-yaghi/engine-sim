#ifndef ATG_ENGINE_SIM_STANDARD_VALVETRAIN_H
#define ATG_ENGINE_SIM_STANDARD_VALVETRAIN_H

#include "valvetrain.h"

class StandardValvetrain : public Valvetrain {
public:
    struct Parameters {
        Camshaft *IntakeCamshaft;
        Camshaft *ExhaustCamshaft;
    };

public:
    StandardValvetrain();
    virtual ~StandardValvetrain();

    void initialize(const Parameters &parameters);

    virtual double intakeValveLift(int cylinder) override;
    virtual double exhaustValveLift(int cylinder) override;

    virtual Camshaft *getActiveIntakeCamshaft() override;
    virtual Camshaft *getActiveExhaustCamshaft() override;

private:
    Camshaft *m_intakeCamshaft;
    Camshaft *m_exhaustCamshaft;
};

#endif /* ATG_ENGINE_SIM_STANDARD_VALVETRAIN_H */
