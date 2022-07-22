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
    };

    template <typename Type>
    extern inline const piranha::ChannelType *LookupChannelType() {
        static_assert(false, "Invalid type lookup");
        return nullptr;
    }

    class EngineNode;
    class RodJournalNode;
    class CrankshaftNode;
    class ConnectingRodNode;
    class CylinderBankNode;
    class PistonNode;
    class FunctionNode;
    class IntakeNode;
    class ExhaustSystemNode;
    class CylinderHeadNode;
    class CamshaftNode;
    class IgnitionModuleNode;
    class IgnitionWireNode;

#define ASSIGN_CHANNEL_TYPE(type, channel) \
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

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CHANNEL_TYPES_H */
