#ifndef ATG_ENGINE_SIM_NODE_H
#define ATG_ENGINE_SIM_NODE_H

#include "piranha.h"

#include <map>
#include <string>

namespace es_script {

    class Node : public piranha::Node {
    protected:
        struct InputTarget {
            enum class Type {
                Object,
                Atomic
            };

            piranha::pNodeInput *input = nullptr;
            void *memoryTarget = nullptr;
            Type type = Type::Atomic;
        };

    public:
        Node() {
            /* void */
        }

        virtual ~Node() {
            for (auto i : m_inputMap) {
                delete i.second.input;
            }
        }

        template <typename T_Out>
        T_Out readAtomicInput(const std::string &name) {
            T_Out out;
            (*m_inputMap[name].input)->fullCompute(&out);

            return out;
        }

        void readAllInputs() {
            for (auto i : m_inputMap) {
                if (i.second.type == InputTarget::Type::Atomic
                    || i.second.type == InputTarget::Type::Object)
                {
                    (*m_inputMap[i.first].input)->fullCompute(i.second.memoryTarget);
                }
            }
        }

        void addInput(
            const std::string &name,
            void *target,
            InputTarget::Type type = InputTarget::Type::Atomic)
        {
            m_inputMap[name] = {
                new piranha::pNodeInput,
                target,
                type
            };
        }

        virtual void registerInputs() {
            for (auto i : m_inputMap) {
                registerInput(i.second.input, i.first);
            }
        }

    private:
        std::map<std::string, InputTarget> m_inputMap;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_NODE_H */
