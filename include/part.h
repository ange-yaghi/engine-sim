#ifndef ATG_ENGINE_SIM_PART_H
#define ATG_ENGINE_SIM_PART_H

#include "scs.h"

class Part {
    public:
        Part();
        virtual ~Part();

        virtual void destroy();

        atg_scs::RigidBody m_body;
};

#endif /* ATG_ENGINE_SIM_PART_H */
