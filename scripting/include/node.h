#ifndef ATG_ENGINE_SIM_NODE_H
#define ATG_ENGINE_SIM_NODE_H

#include "piranha.h"

#include <map>
#include <string>

namespace es_script {
    class Node : public piranha::Node {
    public:
        Node() {
            /* void */
        }

        virtual ~Node() {
            for (auto i : m_inputMap) {
                delete i.second;
            }
        }

        template <typename T_Out>
        T_Out readAtomicInput(const std::string &name) {
            T_Out out;
            (*m_inputMap[name])->fullCompute(&out);

            return out;
        }

        void addInput(const std::string &name) {
            m_inputMap[name] = new piranha::pNodeInput;
        }

        virtual void registerInputs() {
            for (auto i : m_inputMap) {
                registerInput(i.second, i.first);
            }
        }

    private:
        std::map<std::string, piranha::pNodeInput *> m_inputMap;
    };
} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_NODE_H */
