#ifndef ATG_ENGINE_SIM_CYLINDER_BANK_OBJECT_H
#define ATG_ENGINE_SIM_CYLINDER_BANK_OBJECT_H

#include "simulation_object.h"

#include "geometry_generator.h"
#include "cylinder_bank.h"
#include "cylinder_head.h"

class CylinderBankObject : public SimulationObject {
    public:
        CylinderBankObject();
        virtual ~CylinderBankObject();

        virtual void generateGeometry();
        virtual void render(const ViewParameters *view);
        virtual void process(float dt);
        virtual void destroy();

        CylinderBank *m_bank;
        CylinderHead *m_head;
        GeometryGenerator::GeometryIndices m_walls;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_BANK_OBJECT_H */
