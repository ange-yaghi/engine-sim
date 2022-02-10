#ifndef ATG_ENGINE_SIM_PISTON_H
#define ATG_ENGINE_SIM_PISTON_H

#include "part.h"

class ConnectingRod;
class CylinderBank;
class Piston : public Part {
    public:
        struct Parameters {
            ConnectingRod *Rod;
            CylinderBank *Bank;
            int CylinderIndex;

            double CompressionHeight;
            double WristPinPosition;
            double Displacement;
            double Mass;
        };

    public:
        Piston();
        virtual ~Piston();

        void initialize(const Parameters &params);
        virtual void destroy();

        ConnectingRod *m_rod;
        CylinderBank *m_bank;
        int m_cylinderIndex;
        double m_compressionHeight;
        double m_displacement;
        double m_wristPinLocation;
        double m_mass;
};

#endif /* ATG_ENGINE_SIM_PISTON_H */
