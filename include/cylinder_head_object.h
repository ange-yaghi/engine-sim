#ifndef ATG_ENGINE_SIM_CYLINDER_HEAD_OBJECT_H
#define ATG_ENGINE_SIM_CYLINDER_HEAD_OBJECT_H

#include "simulation_object.h"

#include "cylinder_head.h"
#include "geometry_generator.h"

class CylinderHeadObject : public SimulationObject {
    public:
        CylinderHeadObject();
        virtual ~CylinderHeadObject();

        virtual void generateGeometry();
        virtual void render(const ViewParameters *view);
        virtual void process(float dt);
        virtual void destroy();

        CylinderHead *m_head;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_HEAD_OBJECT_H */
