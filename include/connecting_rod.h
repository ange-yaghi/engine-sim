#ifndef ATG_ENGINE_CONNECTING_ROD_H
#define ATG_ENGINE_CONNECTING_ROD_H

#include "part.h"

#include "crankshaft.h"

class ConnectingRod : public Part {
    public:
        struct Parameters {
            double Mass;
            double MomentOfInertia;
            double CenterOfMass;
            double Length;

            Crankshaft *Crankshaft;
            int Journal;
        };

    public:
        ConnectingRod();
        virtual ~ConnectingRod();

        void initialize(const Parameters &params);

        double getBigEndLocal() const;
        double getLittleEndLocal() const;

        double m_centerOfMass;
        double m_length;
        double m_m;
        double m_I;
        int m_journal;
        Crankshaft *m_crankshaft;
};

#endif /* ATG_ENGINE_SIM_CONNECTING_ROD_H */
