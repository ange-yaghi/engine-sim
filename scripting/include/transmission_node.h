#ifndef ATG_ENGINE_SIM_TRANSMISSION_NODE_H
#define ATG_ENGINE_SIM_TRANSMISSION_NODE_H

#include "object_reference_node.h"

#include "engine_sim.h"

#include <vector>

namespace es_script {

    class TransmissionNode : public ObjectReferenceNode<TransmissionNode> {
    public:
        TransmissionNode() { /* void */ }
        virtual ~TransmissionNode() { /* void */ }

        void generate(Transmission *transmission) const {
            Transmission::Parameters parameters = m_parameters;
            parameters.GearCount = static_cast<int>(m_gears.size());
            parameters.GearRatios = m_gears.data();
            parameters.TransmissionType = parseType(m_type);

            transmission->initialize(parameters);
        }

        void addGear(double ratio) {
            m_gears.push_back(ratio);
        }

    protected:
        virtual void registerInputs() {
            addInput("max_clutch_torque", &m_parameters.MaxClutchTorque);
            addInput("straight_cut", &m_parameters.StraightCut);
            addInput("type", &m_type);
            addInput("allow_manual_shift", &m_parameters.AllowManualShift);
            addInput("has_sport_mode", &m_parameters.HasSportMode);
            addInput("shift_up_rpm_closed_throttle", &m_parameters.ShiftUpRpmClosedThrottle);
            addInput("shift_up_rpm_wot", &m_parameters.ShiftUpRpmWOT);
            addInput("shift_down_rpm_closed_throttle", &m_parameters.ShiftDownRpmClosedThrottle);
            addInput("shift_down_rpm_wot", &m_parameters.ShiftDownRpmWOT);
            addInput("sport_shift_up_rpm_closed_throttle", &m_parameters.SportShiftUpRpmClosedThrottle);
            addInput("sport_shift_up_rpm_wot", &m_parameters.SportShiftUpRpmWOT);
            addInput("sport_shift_down_rpm_closed_throttle", &m_parameters.SportShiftDownRpmClosedThrottle);
            addInput("sport_shift_down_rpm_wot", &m_parameters.SportShiftDownRpmWOT);
            addInput("shift_time_drive", &m_parameters.ShiftTimeDrive);
            addInput("shift_time_sport", &m_parameters.ShiftTimeSport);
            addInput("shift_torque_cut", &m_parameters.ShiftTorqueCut);
            addInput("shift_prepare_time_drive", &m_parameters.ShiftPrepareTimeDrive);
            addInput("shift_prepare_time_sport", &m_parameters.ShiftPrepareTimeSport);
            addInput("cvt_target_rpm", &m_parameters.CvtTargetRpm);
            addInput("cvt_ratio_rate", &m_parameters.CvtRatioRate);
            addInput("converter_stall_torque", &m_parameters.ConverterStallTorque);
            addInput("converter_stall_speed", &m_parameters.ConverterStallSpeed);
            addInput("converter_stall_torque_ratio", &m_parameters.ConverterStallTorqueRatio);
            addInput("converter_coupling_point", &m_parameters.ConverterCouplingPoint);
            addInput("has_lockup_clutch", &m_parameters.HasLockupClutch);
            addInput("launch_clutch_creep_capacity", &m_parameters.LaunchClutchCreepCapacity);

            ObjectReferenceNode<TransmissionNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        static Transmission::Type parseType(const std::string &type) {
            if (type == "automatic") return Transmission::Type::AutomaticTorqueConverter;
            if (type == "dual_clutch") return Transmission::Type::DualClutch;
            if (type == "automated_manual") return Transmission::Type::AutomatedManual;
            if (type == "cvt") return Transmission::Type::CVT;
            return Transmission::Type::Manual;
        }

        Transmission::Parameters m_parameters;
        std::vector<double> m_gears;
        std::string m_type = "manual";
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_TRANSMISSION_NODE_H */
