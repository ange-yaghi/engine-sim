#ifndef ATG_ENGINE_SIM_CHANNEL_TYPES_H
#define ATG_ENGINE_SIM_CHANNEL_TYPES_H

#include "piranha.h"

namespace es_script {

    struct ObjectChannel {
        static const piranha::ChannelType EngineChannel;
    };

    template <typename Type>
    extern inline const piranha::ChannelType *LookupChannelType() {
        static_assert(false, "Invalid type lookup");
        return nullptr;
    }

    class EngineNode;

#define ASSIGN_CHANNEL_TYPE(type, channel) template <> extern inline const piranha::ChannelType *LookupChannelType<type>() { return &ObjectChannel::channel; }

    // Register all types
    ASSIGN_CHANNEL_TYPE(EngineNode, EngineChannel);

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CHANNEL_TYPES_H */
