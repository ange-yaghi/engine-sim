#ifndef ATG_ENGINE_CONNECTING_ROD_H
#define ATG_ENGINE_CONNECTING_ROD_H

#include "part.h"

#include "crankshaft.h"

class Piston;
class ConnectingRod : public Part {
    public:
        struct Parameters {
            double mass = 0.0;
            double momentOfInertia = 0.0;
            double centerOfMass = 0.0;
            double length = 0.0;

            int rodJournals = 0;
            double slaveThrow = 0;

            Piston *piston = nullptr;

            Crankshaft *crankshaft = nullptr;
            ConnectingRod *master = nullptr;
            int journal = 0;
        };

    public:
        ConnectingRod();
        virtual ~ConnectingRod();

        void initialize(const Parameters &params);

        double getBigEndLocal() const;
        double getLittleEndLocal() const;

        void setMaster(ConnectingRod *rod) { m_master = rod; }
        void setCrankshaft(Crankshaft *crank) { m_crankshaft = crank; }

        inline int getRodJournalCount() const { return m_rodJournalCount; }
        void setRodJournalAngle(int i, double angle);
        void getRodJournalPositionLocal(int i, double *x, double *y);
        void getRodJournalPositionGlobal(int i, double *x, double *y);
        double getRodJournalAngle(int i) { return m_rodJournalAngles[i]; }

        inline double getSlaveThrow() const { return m_slaveThrow; }
        inline double getCenterOfMass() const { return m_centerOfMass; }
        inline double getLength() const { return m_length; }
        inline double getMass() const { return m_m; }
        inline double getMomentOfInertia() const { return m_I; }
        inline int getJournal() const { return m_journal; }
        int getLayer() const;
        inline ConnectingRod *getMasterRod() const { return m_master; }
        inline Crankshaft *getCrankshaft() const { return m_crankshaft; }
        inline Piston *getPiston() const { return m_piston; }

    protected:
        double m_centerOfMass;
        double m_length;
        double m_m;
        double m_I;
        int m_journal;
        ConnectingRod *m_master;
        Crankshaft *m_crankshaft;
        Piston *m_piston;

        double m_slaveThrow;
        double *m_rodJournalAngles;
        int m_rodJournalCount;
};

#endif /* ATG_ENGINE_SIM_CONNECTING_ROD_H */
