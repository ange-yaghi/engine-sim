#ifndef ATG_ENGINE_SIM_CHANNEL_TYPES_H
#define ATG_ENGINE_SIM_CHANNEL_TYPES_H

#include "piranha.h"

namespace es_script {

    struct ObjectChannel {
        static const piranha::ChannelType EngineChannel;
        static const piranha::ChannelType CrankshaftChannel;
        static const piranha::ChannelType RodJournalChannel;
        static const piranha::ChannelType ConnectingRodChannel;
        static const piranha::ChannelType CylinderBankChannel;
        static const piranha::ChannelType PistonChannel;
        static const piranha::ChannelType FunctionChannel;
        static const piranha::ChannelType IntakeChannel;
        static const piranha::ChannelType ExhaustSystemChannel;
        static const piranha::ChannelType CylinderHeadChannel;
        static const piranha::ChannelType CamshaftChannel;
        static const piranha::ChannelType IgnitionModuleChannel;
        static const piranha::ChannelType IgnitionWireChannel;
        static const piranha::ChannelType FuelChannel;
        static const piranha::ChannelType ImpulseResponseChannel;
        static const piranha::ChannelType ValvetrainChannel;
        static const piranha::ChannelType VehicleChannel;
        static const piranha::ChannelType TransmissionChannel;
        static const piranha::ChannelType ThrottleChannel;
    };

    template <typename Type>
    extern inline const piranha::ChannelType *LookupChannelType() {
        static_assert(false, "Invalid type lookup");
        return nullptr;
    }

#define ASSIGN_CHANNEL_TYPE(type, channel) \
    class type; \
    template <> extern inline const piranha::ChannelType *LookupChannelType<type>() { \
        return &ObjectChannel::channel; \
    }

    // Register all types
    ASSIGN_CHANNEL_TYPE(EngineNode, EngineChannel);
    ASSIGN_CHANNEL_TYPE(CrankshaftNode, CrankshaftChannel);
    ASSIGN_CHANNEL_TYPE(RodJournalNode, RodJournalChannel);
    ASSIGN_CHANNEL_TYPE(ConnectingRodNode, ConnectingRodChannel);
    ASSIGN_CHANNEL_TYPE(CylinderBankNode, CylinderBankChannel);
    ASSIGN_CHANNEL_TYPE(PistonNode, PistonChannel);
    ASSIGN_CHANNEL_TYPE(FunctionNode, FunctionChannel);
    ASSIGN_CHANNEL_TYPE(IntakeNode, IntakeChannel);
    ASSIGN_CHANNEL_TYPE(ExhaustSystemNode, ExhaustSystemChannel);
    ASSIGN_CHANNEL_TYPE(CylinderHeadNode, CylinderHeadChannel);
    ASSIGN_CHANNEL_TYPE(CamshaftNode, CamshaftChannel);
    ASSIGN_CHANNEL_TYPE(IgnitionModuleNode, IgnitionModuleChannel);
    ASSIGN_CHANNEL_TYPE(IgnitionWireNode, IgnitionWireChannel);
    ASSIGN_CHANNEL_TYPE(FuelNode, FuelChannel);
    ASSIGN_CHANNEL_TYPE(ImpulseResponseNode, ImpulseResponseChannel);
    ASSIGN_CHANNEL_TYPE(ValvetrainNode, ValvetrainChannel);
    ASSIGN_CHANNEL_TYPE(VehicleNode, VehicleChannel);
    ASSIGN_CHANNEL_TYPE(TransmissionNode, VehicleChannel);
    ASSIGN_CHANNEL_TYPE(ThrottleNode, ThrottleChannel);

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CHANNEL_TYPES_H */
