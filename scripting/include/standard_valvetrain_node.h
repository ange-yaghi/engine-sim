#ifndef ATG_ENGINE_SIM_STANDARD_VALVETRAIN_NODE_H
#define ATG_ENGINE_SIM_STANDARD_VALVETRAIN_NODE_H

#include "valvetrain_node.h"

#include "camshaft_node.h"

#include "engine_sim.h"

namespace es_script {

    class StandardValvetrainNode : public ValvetrainNode {
    public:
        StandardValvetrainNode() { /* void */ }
        virtual ~StandardValvetrainNode() { /* void */ }

        virtual Valvetrain *generate(
            EngineContext *context,
            Crankshaft *crank) override
        {
            StandardValvetrain *valvetrain = new StandardValvetrain;

            Camshaft
                *intakeCam = new Camshaft(),
                *exhaustCam = new Camshaft();

            m_intakeCamshaft->generate(intakeCam, crank, context);
            m_exhaustCamshaft->generate(exhaustCam, crank, context);

            StandardValvetrain::Parameters params;
            params.IntakeCamshaft = intakeCam;
            params.ExhaustCamshaft = exhaustCam;
            valvetrain->initialize(params);

            return valvetrain;
        }

    protected:
        virtual void registerInputs() {
            addInput("intake_camshaft", &m_intakeCamshaft, InputTarget::Type::Object);
            addInput("exhaust_camshaft", &m_exhaustCamshaft, InputTarget::Type::Object);

            ValvetrainNode::registerInputs();
        }

    private:
        CamshaftNode *m_intakeCamshaft = nullptr;
        CamshaftNode *m_exhaustCamshaft = nullptr;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_STANDARD_VALVETRAIN_NODE_H */
