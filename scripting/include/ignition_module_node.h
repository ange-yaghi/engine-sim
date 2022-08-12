#ifndef ATG_ENGINE_SIM_IGNITION_MODULE_NODE_H
#define ATG_ENGINE_SIM_IGNITION_MODULE_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "function_node.h"

#include "engine_sim.h"

#include <map>

namespace es_script {

    class IgnitionModuleNode : public ObjectReferenceNode<IgnitionModuleNode> {
    public:
        struct Post {
            IgnitionWireNode *wire;
            double angle;
        };

    public:
        IgnitionModuleNode() { /* void */ }
        virtual ~IgnitionModuleNode() { /* void */ }

        void generate(Engine *engine, EngineContext *context) const {
            IgnitionModule::Parameters params;
            params.Crankshaft = engine->getCrankshaft(0);
            params.CylinderCount = engine->getCylinderCount();
            params.RevLimit = m_revLimit;
            params.TimingCurve = m_timingCurve->generate(context);
            params.CylinderCount = engine->getCylinderCount();
            params.LimiterDuration = m_limiterDuration;
            engine->getIgnitionModule()->initialize(params);

            for (const Post &post : m_posts) {
                std::set<IgnitionWireNode::Connection> connections =
                    post.wire->getConnections();
                for (const IgnitionWireNode::Connection &connection : connections) {
                    const int index = context->getCylinderIndex(
                        connection.first,
                        connection.second
                    );

                    engine->getIgnitionModule()->setFiringOrder(index, post.angle);
                }
            }
        }

        void connect(IgnitionWireNode *wire, double angle) {
            m_posts.push_back({ wire, angle });
        }

    protected:
        virtual void registerInputs() {
            addInput("timing_curve", &m_timingCurve);
            addInput("rev_limit", &m_revLimit);
            addInput("limiter_duration", &m_limiterDuration);

            ObjectReferenceNode<IgnitionModuleNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        double m_revLimit = 0.0;
        FunctionNode *m_timingCurve = nullptr;
        double m_limiterDuration = 0.0;
        std::vector<Post> m_posts;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_IGNITION_MODULE_NODE_H */
