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

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CHANNEL_TYPES_H */
