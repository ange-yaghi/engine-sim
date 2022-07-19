#ifndef ATG_ENGINE_SIM_CAMSHAFT_NODE_H
#define ATG_ENGINE_SIM_CAMSHAFT_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "function_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class CamshaftNode : public ObjectReferenceNode<CamshaftNode> {
    public:
        CamshaftNode() { /* void */ }
        virtual ~CamshaftNode() { /* void */ }

        void generate(
            Camshaft *camshaft,
            Crankshaft *crankshaft,
            EngineContext *context) const
        {
            Camshaft::Parameters parameters = m_parameters;
            parameters.Crankshaft = crankshaft;
            parameters.Lobes = (int)m_lobes.size();
            parameters.LobeProfile = m_lobeProfile->generate(context);
            
            camshaft->initialize(parameters);

            for (int i = 0; i < parameters.Lobes; ++i) {
                camshaft->setLobeCenterline(i, m_lobes[i]);
            }
        }

        void addLobe(double lobeCenterline) {
            m_lobes.push_back(lobeCenterline);
        }

    protected:
        virtual void registerInputs() {
            addInput("advance", &m_parameters.Advance);
            addInput("base_radius", &m_parameters.BaseRadius);
            addInput("lobe_profile", &m_lobeProfile);

            ObjectReferenceNode<CamshaftNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.Crankshaft = nullptr;
        }

        Camshaft::Parameters m_parameters;
        FunctionNode *m_lobeProfile = nullptr;
        std::vector<double> m_lobes;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CAMSHAFT_NODE_H */
