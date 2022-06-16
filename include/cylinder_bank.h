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

        void getPositionAboveDeck(double h, double *x, double *y) const;
        double boreSurfaceArea() const;

        inline double getAngle() const { return m_angle; }
        inline double getBore() const { return m_bore; }
        inline double getDeckHeight() const { return m_deckHeight; }
        inline int getCylinderCount() const { return m_cylinderCount; }
        inline int getIndex() const { return m_index; }
        inline double getDx() const { return m_dx; }
        inline double getDy() const { return m_dy; }

    protected:
        double m_angle;
        double m_bore;
        double m_deckHeight;
        int m_cylinderCount;
        int m_index;

        double m_dx;
        double m_dy;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_BANK_H */
