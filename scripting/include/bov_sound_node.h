#ifndef ATG_ENGINE_SIM_BOV_SOUND_NODE_H
#define ATG_ENGINE_SIM_BOV_SOUND_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "bov_sound_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class BovSoundNode
        : public ObjectReferenceNode<BovSoundNode> {
    public:
        BovSoundNode() { /* void */ }
        virtual ~BovSoundNode() { /* void */ }

        BovSound *generate(EngineContext *context) {
            BovSound *existing = context->getBovSound(this);
            if (existing != nullptr) {
                return existing;
            }
            else {
                piranha::Path path = m_filename;
                piranha::Path parentPath;
                m_irStructure->getParentUnit()->getPath().getParentPath(&parentPath);
                if (!path.isAbsolute()) {
                    path = parentPath.append(path);
                }

                BovSound *bovSound = new BovSound;
                bovSound->initialize(
                    path.toString(),
                    m_volume);

                return bovSound;
            }
        }

    protected:
        virtual void registerInputs() {
            addInput("filename", &m_filename);
            addInput("volume", &m_volume);

            ObjectReferenceNode<BovSoundNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        std::string m_filename = "";
        double m_volume = 1.0;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_BOV_SOUND_NODE_H */
