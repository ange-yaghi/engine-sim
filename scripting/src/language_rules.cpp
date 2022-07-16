#include "../include/language_rules.h"

#include "../include/channel_types.h"
#include "../include/engine_node.h"
#include "../include/actions.h"

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

    // Int operations
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_int,
        piranha::MultiplyOperationNodeOutput>>("__engine_sim__int_multiply");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_int,
        piranha::AddOperationNodeOutput>>("__engine_sim__int_add");
    registerBuiltinType<piranha::OperationNodeSpecialized<
        piranha::native_int,
        piranha::SubtractOperationNodeOutput>>("__engine_sim__int_sub");
    registerBuiltinType<piranha::NumNegateOperationNode<
        piranha::native_int>>("__engine_sim__int_negate");

    registerBuiltinType<SetEngineNode>("__engine_sim__set_engine");
    registerBuiltinType<EngineNode>("__engine_sim__engine");

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
}
