#ifndef ATG_ENGINE_SIM_CYLINDER_BANK_H
#define ATG_ENGINE_SIM_CYLINDER_BANK_H

#include "part.h"

class CylinderBank {
    public:
        struct Parameters {
            double Angle;
            double Bore;
            double DeckHeight;
            int CylinderCount;
            int Index;
        };

    public:
        CylinderBank();
        ~CylinderBank();

        void initialize(const Parameters &params);
        void destroy();

        double m_angle;
        double m_bore;
        double m_deckHeight;
        int m_cylinderCount;
        int m_index;

        double m_dx;
        double m_dy;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_BANK_H */
