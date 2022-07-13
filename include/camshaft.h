#ifndef ATG_ENGINE_SIM_CAMSHAFT_H
#define ATG_ENGINE_SIM_CAMSHAFT_H

#include "part.h"

#include "function.h"
#include "units.h"

class Crankshaft;
class Camshaft : public Part {
    public:
        struct Parameters {
            // Number of lobes
            int Lobes;

            // Camshaft advance in camshaft degrees
            double Advance = 0;

            // Corresponding crankshaft
            Crankshaft *Crankshaft;

            // Lobe profile
            Function *LobeProfile;

            // Base radius
            double BaseRadius = units::distance(600, units::thou);
        };

    public:
        Camshaft();
        virtual ~Camshaft();

        void initialize(const Parameters &params);
        virtual void destroy();

        double valveLift(int lobe) const;
        double sampleLobe(double theta) const;

        void setLobeCenterline(int lobe, double crankAngle) { m_lobeAngles[lobe] = crankAngle / 2; }
        double getLobeCenterline(int lobe) const { return m_lobeAngles[lobe]; }

        double getAngle() const;

        Function *getLobeProfile() const { return m_lobeProfile; }
        double getAdvance() const { return m_advance; }
        double getBaseRadius() const { return m_baseRadius; }

    private:
        Crankshaft *m_crankshaft;
        Function *m_lobeProfile;
        double *m_lobeAngles;
        double m_advance;
        double m_baseRadius;
        int m_lobes;
};

#endif /* ATG_ENGINE_SIM_CAMSHAFT_H */
