#ifndef ATG_ENGINE_SIM_CYLINDER_BANK_H
#define ATG_ENGINE_SIM_CYLINDER_BANK_H

#include "part.h"

#include "crankshaft.h"

class CylinderBank {
    public:
        struct Parameters {
            Crankshaft *crankshaft;
            double positionX;
            double positionY;
            double angle;
            double bore;
            double deckHeight;
            double displayDepth;
            int cylinderCount;
            int index;
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
        inline double getX() const { return m_x; }
        inline double getY() const { return m_y; }
        inline double getDisplayDepth() const { return m_displayDepth; }

    protected:
        double m_angle;
        double m_bore;
        double m_deckHeight;
        double m_displayDepth;
        int m_cylinderCount;
        int m_index;

        double m_dx;
        double m_dy;
        double m_x;
        double m_y;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_BANK_H */
