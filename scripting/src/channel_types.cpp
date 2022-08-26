#include "../include/channel_types.h"

#define DEFINE_CHANNEL(channel_type) const piranha::ChannelType es_script::ObjectChannel::channel_type(#channel_type);

DEFINE_CHANNEL(EngineChannel);
DEFINE_CHANNEL(CrankshaftChannel);
DEFINE_CHANNEL(RodJournalChannel);
DEFINE_CHANNEL(ConnectingRodChannel);
DEFINE_CHANNEL(CylinderBankChannel);
DEFINE_CHANNEL(PistonChannel);
DEFINE_CHANNEL(FunctionChannel);
DEFINE_CHANNEL(IntakeChannel);
DEFINE_CHANNEL(ExhaustSystemChannel);
DEFINE_CHANNEL(CylinderHeadChannel);
DEFINE_CHANNEL(CamshaftChannel);
DEFINE_CHANNEL(IgnitionModuleChannel);
DEFINE_CHANNEL(IgnitionWireChannel);
DEFINE_CHANNEL(FuelChannel);
DEFINE_CHANNEL(ImpulseResponseChannel);
DEFINE_CHANNEL(ValvetrainChannel);
DEFINE_CHANNEL(VehicleChannel);
DEFINE_CHANNEL(TransmissionChannel);
