#ifndef ATG_ENGINE_SIM_CAMSHAFT_H
#define ATG_ENGINE_SIM_CAMSHAFT_H

#include "part.h"

#include "function.h"

class Crankshaft;
class Camshaft : public Part {
    public:
        struct Parameters {
            int Lobes;
            Crankshaft *Crankshaft;
            Function *LobeProfile;
        };

    public:
        Camshaft();
        virtual ~Camshaft();

        void initialize(const Parameters &params);
        virtual void destroy();

        double valveLift(int lobe) const;
        double sampleLobe(double theta) const;

    protected:
        double getAngle() const;

        Crankshaft *m_crankshaft;
        Function *m_lobeProfile;
        double *m_lobeAngles;
        int m_lobes;
};

#endif /* ATG_ENGINE_SIM_CAMSHAFT_H */
