#ifndef ATG_ENGINE_CONNECTING_ROD_H
#define ATG_ENGINE_CONNECTING_ROD_H

#include "part.h"

#include "crankshaft.h"

class Piston;
class ConnectingRod : public Part {
    public:
        struct Parameters {
            double Mass = 0.0;
            double MomentOfInertia = 0.0;
            double CenterOfMass = 0.0;
            double Length = 0.0;

            Crankshaft *Crankshaft = nullptr;
            Piston *Piston = nullptr;
            int Journal = 0;
        };

    public:
        ConnectingRod();
        virtual ~ConnectingRod();

        void initialize(const Parameters &params);

        double getBigEndLocal() const;
        double getLittleEndLocal() const;

        inline double getCenterOfMass() const { return m_centerOfMass; }
        inline double getLength() const { return m_length; }
        inline double getMass() const { return m_m; }
        inline double getMomentOfInertia() const { return m_I; }
        inline int getJournal() const { return m_journal; }
        inline Crankshaft *getCrankshaft() const { return m_crankshaft; }
        inline Piston *getPiston() const { return m_piston; }

    protected:
        double m_centerOfMass;
        double m_length;
        double m_m;
        double m_I;
        int m_journal;
        Crankshaft *m_crankshaft;
        Piston *m_piston;
};

#endif /* ATG_ENGINE_SIM_CONNECTING_ROD_H */
