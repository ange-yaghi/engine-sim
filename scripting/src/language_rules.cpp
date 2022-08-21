#include "../include/language_rules.h"

#include "../include/channel_types.h"
#include "../include/engine_node.h"
#include "../include/actions.h"
#include "../include/rod_journal_node.h"
#include "../include/camshaft_node.h"
#include "../include/cylinder_head_node.h"
#include "../include/ignition_module_node.h"
#include "../include/impulse_response_node.h"
#include "../include/standard_valvetrain_node.h"
#include "../include/vtec_valvetrain_node.h"
#include "../include/set_application_settings_node.h"

es_script::LanguageRules::LanguageRules() {
    /* void */
}

es_script::LanguageRules::~LanguageRules() {
    /* void */
}

void es_script::LanguageRules::registerBuiltinNodeTypes() {
    // ====================================================
    // Builtin types
    // ====================================================

    // Channels
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__float", &piranha::FundamentalType::FloatType);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__int", &piranha::FundamentalType::IntType);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__bool", &piranha::FundamentalType::BoolType);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__string", &piranha::FundamentalType::StringType);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__engine_channel", &es_script::ObjectChannel::EngineChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__crankshaft_channel", &es_script::ObjectChannel::CrankshaftChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__rod_journal_channel", &es_script::ObjectChannel::RodJournalChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__connecting_rod_channel", &es_script::ObjectChannel::ConnectingRodChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__piston_channel", &es_script::ObjectChannel::PistonChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__cylinder_bank_channel", &es_script::ObjectChannel::CylinderBankChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__function_channel", &es_script::ObjectChannel::FunctionChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__cylinder_head_channel", &es_script::ObjectChannel::CylinderHeadChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__camshaft_channel", &es_script::ObjectChannel::CamshaftChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__intake_channel", &es_script::ObjectChannel::IntakeChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__exhaust_system_channel", &es_script::ObjectChannel::ExhaustSystemChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__ignition_module_channel", &es_script::ObjectChannel::IgnitionModuleChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__ignition_wire_channel", &es_script::ObjectChannel::IgnitionWireChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__fuel_channel", &es_script::ObjectChannel::FuelChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__impulse_response_channel", &es_script::ObjectChannel::ImpulseResponseChannel);
    registerBuiltinType<piranha::ChannelNode>(
        "__engine_sim__valvetrain_channel", &es_script::ObjectChannel::ValvetrainChannel);

    // Literals
    registerBuiltinType<piranha::DefaultLiteralFloatNode>(
        "__engine_sim__literal_float", &piranha::FundamentalType::FloatType);
    registerBuiltinType<piranha::DefaultLiteralStringNode>(
        "__engine_sim__literal_string", &piranha::FundamentalType::StringType);
    registerBuiltinType<piranha::DefaultLiteralIntNode>(
        "__engine_sim__literal_int", &piranha::FundamentalType::IntType);
    registerBuiltinType<piranha::DefaultLiteralBoolNode>(
        "__engine_sim__literal_bool", &piranha::FundamentalType::BoolType);

    // Conversions
    registerBuiltinType<piranha::IntToFloatConversionNode>(
        "__engine_sim__int_to_float");
    registerBuiltinType<piranha::IntToStringConversionNode>(
        "__engine_sim__int_to_string");
    registerBuiltinType<piranha::StringToIntConversionNode>(
        "__engine_sim__string_to_int");

    // Float operations
    registerBuiltinType<piranha::NumNegateOperationNode<piranha::native_float>>(
        "__engine_sim__float_negate");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_float,
        piranha::DivideOperationNodeOutput>>("__engine_sim__float_divide");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_float,
        piranha::MultiplyOperationNodeOutput>>("__engine_sim__float_multiply");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_float,
        piranha::DivideOperationNodeOutput>>("__engine_sim__float_divide");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_float,
        piranha::AddOperationNodeOutput>>("__engine_sim__float_add");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_float,
        piranha::SubtractOperationNodeOutput>>("__engine_sim__float_subtract");

    // Int operations
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_int,
        piranha::MultiplyOperationNodeOutput>>("__engine_sim__int_multiply");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_int,
        piranha::AddOperationNodeOutput>>("__engine_sim__int_add");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_int,
        piranha::SubtractOperationNodeOutput>>("__engine_sim__int_subtract");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_int,
        piranha::SubtractOperationNodeOutput>>("__engine_sim__int_divide");
    registerBuiltinType<piranha::NumNegateOperationNode<
        piranha::native_int>>("__engine_sim__int_negate");

    // Actions
    registerBuiltinType<SetEngineNode>("__engine_sim__set_engine");
    registerBuiltinType<AddRodJournalNode>("__engine_sim__add_rod_journal");
    registerBuiltinType<AddCrankshaftNode>("__engine_sim__add_crankshaft");
    registerBuiltinType<AddCylinderBankNode>("__engine_sim__add_cylinder_bank");
    registerBuiltinType<AddCylinderNode>("__engine_sim__add_cylinder");
    registerBuiltinType<AddSampleNode>("__engine_sim__add_sample");
    registerBuiltinType<AddLobeNode>("__engine_sim__add_lobe");
    registerBuiltinType<SetCylinderHeadNode>("__engine_sim__set_cylinder_head");
    registerBuiltinType<ConnectIgnitionWireNode>("__engine_sim__connect_ignition_wire");
    registerBuiltinType<AddIgnitionModuleNode>("__engine_sim__add_ignition_module");
    registerBuiltinType<k_28inH2ONode>("__engine_sim__k_28inH2O");
    registerBuiltinType<k_CarbNode>("__engine_sim__k_carb");
    registerBuiltinType<GenerateHarmonicCamLobe>("__engine_sim__generate_harmonic_cam_lobe");
    registerBuiltinType<SetApplicationSettings>("__engine_sim__set_application_settings");

    // Objects
    registerBuiltinType<EngineNode>("__engine_sim__engine");
    registerBuiltinType<RodJournalNode>("__engine_sim__rod_journal");
    registerBuiltinType<CrankshaftNode>("__engine_sim__crankshaft");
    registerBuiltinType<ConnectingRodNode>("__engine_sim__connecting_rod");
    registerBuiltinType<CylinderBankNode>("__engine_sim__cylinder_bank");
    registerBuiltinType<PistonNode>("__engine_sim__piston");
    registerBuiltinType<FunctionNode>("__engine_sim__function");
    registerBuiltinType<CylinderHeadNode>("__engine_sim__cylinder_head");
    registerBuiltinType<CamshaftNode>("__engine_sim__camshaft");
    registerBuiltinType<ExhaustSystemNode>("__engine_sim__exhaust_system");
    registerBuiltinType<IntakeNode>("__engine_sim__intake");
    registerBuiltinType<IgnitionModuleNode>("__engine_sim__ignition_module");
    registerBuiltinType<IgnitionWireNode>("__engine_sim__ignition_wire");
    registerBuiltinType<FuelNode>("__engine_sim__fuel");
    registerBuiltinType<ImpulseResponseNode>("__engine_sim__impulse_response");
    registerBuiltinType<StandardValvetrainNode>("__engine_sim__standard_valvetrain");
    registerBuiltinType<VtecValvetrainNode>("__engine_sim__vtec_valvetrain");

    // String operations
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_string,
        piranha::AddOperationNodeOutput>>("__engine_sim__string_add");

    // ====================================================
    // Conversions
    // ====================================================

    registerConversion(
        { &piranha::FundamentalType::IntType, &piranha::FundamentalType::FloatType },
        "__engine_sim__int_to_float"
    );
    registerConversion(
        { &piranha::FundamentalType::IntType, &piranha::FundamentalType::StringType },
        "__engine_sim__int_to_string"
    );
    registerConversion(
        { &piranha::FundamentalType::StringType, &piranha::FundamentalType::IntType },
        "__engine_sim__string_to_int"
    );

    // Literals
    registerLiteralType(piranha::LiteralType::Float, "__engine_sim__literal_float");
    registerLiteralType(piranha::LiteralType::String, "__engine_sim__literal_string");
    registerLiteralType(piranha::LiteralType::Integer, "__engine_sim__literal_int");
    registerLiteralType(piranha::LiteralType::Boolean, "__engine_sim__literal_bool");

    // Operations
    registerUnaryOperator(
        {
            piranha::IrUnaryOperator::Operator::NumericNegate,
            &piranha::FundamentalType::FloatType
        },
        "__engine_sim__float_negate");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Mul,
            &piranha::FundamentalType::FloatType,
            &piranha::FundamentalType::FloatType
        },
        "__engine_sim__float_multiply");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Mul,
            &piranha::FundamentalType::FloatType,
            &piranha::FundamentalType::IntType
        },
        "__engine_sim__float_multiply");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Div,
            &piranha::FundamentalType::FloatType,
            &piranha::FundamentalType::FloatType
        },
        "__engine_sim__float_divide");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Div,
            &piranha::FundamentalType::FloatType,
            &piranha::FundamentalType::IntType
        },
        "__engine_sim__float_divide");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Sub,
            &piranha::FundamentalType::FloatType,
            &piranha::FundamentalType::FloatType
        },
        "__engine_sim__float_subtract");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Sub,
            &piranha::FundamentalType::FloatType,
            &piranha::FundamentalType::IntType
        },
        "__engine_sim__float_subtract");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Add,
            &piranha::FundamentalType::FloatType,
            &piranha::FundamentalType::FloatType
        },
        "__engine_sim__float_add");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Add,
            &piranha::FundamentalType::FloatType,
            &piranha::FundamentalType::IntType
        },
        "__engine_sim__float_add");

    registerUnaryOperator(
        {
            piranha::IrUnaryOperator::Operator::NumericNegate,
            &piranha::FundamentalType::IntType
        },
        "__engine_sim__int_negate");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Mul,
            &piranha::FundamentalType::IntType,
            &piranha::FundamentalType::IntType
        },
        "__engine_sim__int_multiply");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Div,
            &piranha::FundamentalType::IntType,
            &piranha::FundamentalType::IntType
        },
        "__engine_sim__int_divide");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Sub,
            &piranha::FundamentalType::IntType,
            &piranha::FundamentalType::IntType
        },
        "__engine_sim__int_subtract");
    registerOperator(
        {
            piranha::IrBinaryOperator::Operator::Add,
            &piranha::FundamentalType::IntType,
            &piranha::FundamentalType::IntType
        },
        "__engine_sim__int_add");
}
