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

            double BlowbyFlowCoefficient;
            double CompressionHeight;
            double WristPinPosition;
            double Displacement;
            double Mass;
        };

    public:
        Piston();
        virtual ~Piston();

        void initialize(const Parameters &params);
        inline void setCylinderConstraint(atg_scs::LineConstraint *constraint);
        virtual void destroy();

        double relativeX() const;
        double relativeY() const;

        double calculateCylinderWallForce() const;
        inline ConnectingRod *getRod() const { return m_rod; }
        inline CylinderBank *getCylinderBank() const { return m_bank; }
        inline int getCylinderIndex() const { return m_cylinderIndex; }
        inline double getCompressionHeight() const { return m_compressionHeight; }
        inline double getDisplacement() const { return m_displacement; }
        inline double getWristPinLocation() const { return m_wristPinLocation; }
        inline double getMass() const { return m_mass; }
        inline double getBlowbyK() const { return m_blowby_k; }

    protected:
        ConnectingRod *m_rod;
        CylinderBank *m_bank;
        atg_scs::LineConstraint *m_cylinderConstraint;
        int m_cylinderIndex;
        double m_compressionHeight;
        double m_displacement;
        double m_wristPinLocation;
        double m_mass;
        double m_blowby_k;
};

void Piston::setCylinderConstraint(atg_scs::LineConstraint *constraint) {
    m_cylinderConstraint = constraint;
}

#endif /* ATG_ENGINE_SIM_PISTON_H */
