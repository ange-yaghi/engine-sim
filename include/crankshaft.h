#ifndef ATG_ENGINE_SIM_CRANKSHAFT_H
#define ATG_ENGINE_SIM_CRANKSHAFT_H

#include "part.h"

class Crankshaft : public Part {
    public:
        struct Parameters {
            double Mass;
            double FlywheelMass;
            double MomentOfInertia;
            double CrankThrow;
            double Pos_x = 0;
            double Pos_y = 0;
            double TDC = 0;
            int RodJournals;
        };

    public:
        Crankshaft();
        virtual ~Crankshaft();

        void initialize(Parameters &params);
        virtual void destroy();
        void setRodJournalAngle(int i, double angle);
        void getRodJournalPositionLocal(int i, double *x, double *y);

        double getAngle() const;
        double getCycleAngle() const;

        double *m_rodJournalAngles;
        int m_rodJournalCount;

        double m_tdc;
        double m_throw;
        double m_m;
        double m_I;
        double m_flywheelMass;
        double m_p_x;
        double m_p_y;
};

#endif /* ATG_ENGINE_SIM_CRANKSHAFT_H */
