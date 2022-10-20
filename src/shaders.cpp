#include "../include/shaders.h"

Shaders::Shaders() {
    m_cameraPosition = ysMath::LoadVector(0.0f, 0.0f);

    m_mainStage = nullptr;
    m_uiStage = nullptr;

    m_objectVariables.ColorReplace = 1;
    m_objectVariables.Lit = 0;
    m_objectVariables.Transform = ysMath::LoadIdentity();

    m_screenVariables.FogNear = m_uiScreenVariables.FogNear = 16000.0f;
    m_screenVariables.FogFar = m_uiScreenVariables.FogFar = 16001.0f;
}

Shaders::~Shaders() {
    /* void */
}

ysError Shaders::Initialize(
        dbasic::ShaderSet *shaderSet,
        ysRenderTarget *mainRenderTarget,
        ysRenderTarget *uiRenderTarget,
        ysShaderProgram *shaderProgram,
        ysInputLayout *inputLayout)
{
    YDS_ERROR_DECLARE("Initialize");

    YDS_NESTED_ERROR_CALL(shaderSet->NewStage("ShaderStage::Main", &m_mainStage));
    YDS_NESTED_ERROR_CALL(shaderSet->NewStage("ShaderStage::UI", &m_uiStage));

    m_mainStage->SetInputLayout(inputLayout);
    m_mainStage->SetRenderTarget(mainRenderTarget);
    m_mainStage->SetShaderProgram(shaderProgram);
    m_mainStage->SetFlagBit(0);
    m_mainStage->SetType(dbasic::ShaderStage::Type::FullPass);

    m_mainStage->NewConstantBuffer<dbasic::ShaderScreenVariables>(
        "Buffer::ScreenData",
        0,
        dbasic::ShaderStage::ConstantBufferBinding::BufferType::SceneData,
        &m_screenVariables);
    m_mainStage->NewConstantBuffer<dbasic::ShaderObjectVariables>(
        "Buffer::ObjectData",
        1,
        dbasic::ShaderStage::ConstantBufferBinding::BufferType::ObjectData,
        &m_objectVariables);
    m_mainStage->NewConstantBuffer<dbasic::LightingControls>(
        "Buffer::LightingData",
        3,
        dbasic::ShaderStage::ConstantBufferBinding::BufferType::SceneData,
        &m_lightingControls);

    // UI Stage
    m_uiStage->SetInputLayout(inputLayout);
    m_uiStage->SetRenderTarget(uiRenderTarget);
    m_uiStage->SetShaderProgram(shaderProgram);
    m_uiStage->SetFlagBit(1);
    m_uiStage->SetClearTarget(false);
    m_uiStage->SetType(dbasic::ShaderStage::Type::FullPass);

    m_uiStage->NewConstantBuffer<dbasic::ShaderScreenVariables>(
        "Buffer::ScreenData",
        0,
        dbasic::ShaderStage::ConstantBufferBinding::BufferType::SceneData,
        &m_uiScreenVariables);
    m_uiStage->NewConstantBuffer<dbasic::ShaderObjectVariables>(
        "Buffer::ObjectData",
        1,
        dbasic::ShaderStage::ConstantBufferBinding::BufferType::ObjectData,
        &m_objectVariables);
    m_uiStage->NewConstantBuffer<dbasic::LightingControls>(
        "Buffer::LightingData",
        3,
        dbasic::ShaderStage::ConstantBufferBinding::BufferType::SceneData,
        &m_lightingControls);

    return YDS_ERROR_RETURN(ysError::None);
}

ysError Shaders::UseMaterial(dbasic::Material *material) {
    YDS_ERROR_DECLARE("UseMaterial");
    return YDS_ERROR_RETURN(ysError::None);
}

void Shaders::SetObjectTransform(const ysMatrix &mat) {
    m_objectVariables.Transform = mat;
}

void Shaders::ConfigureModel(float scale, dbasic::ModelAsset *model) {
    /* void */
}

void Shaders::SetBaseColor(const ysVector &color) {
    m_objectVariables.BaseColor = color;
}

void Shaders::ResetBaseColor() {
    m_objectVariables.BaseColor = ysMath::LoadVector(1.0f, 1.0f, 1.0f, 1.0f);
}

dbasic::StageEnableFlags Shaders::GetRegularFlags() const {
    return m_mainStage->GetFlags();
}

dbasic::StageEnableFlags Shaders::GetUiFlags() const {
    return m_uiStage->GetFlags();
}

void Shaders::CalculateCamera(
    float width,
    float height,
    const Bounds &cameraBounds,
    float screenWidth,
    float screenHeight,
    float angle)
{
    const ysMatrix projection = ysMath::OrthographicProjection(
        width,
        height,
        0.001f,
        500.0f);
    const Point scale = Point(screenWidth, screenHeight);
    const Point center =
        (cameraBounds.getPosition() - Point(screenWidth / 2, screenHeight / 2))
        / scale;

    m_screenVariables.Projection = ysMath::Transpose(
        ysMath::MatMult(
            projection,
            ysMath::MatMult(
                ysMath::ScaleTransform(ysMath::LoadVector(
                    cameraBounds.width() / screenWidth,
                    cameraBounds.height() / screenHeight,
                    1.0f)),
                ysMath::TranslationTransform(ysMath::LoadVector(center.x, center.y, 0.0f))
            )
        )
    );

    m_screenVariables.Projection = ysMath::Transpose(projection);

    const float sinAngle = std::sin(angle);
    const float cosAngle = std::cos(angle);

    const ysVector cameraEye =
        ysMath::Add(
                ysMath::LoadVector(10.0f * sinAngle, 0.0f, 10.0f * cosAngle, 1.0f),
                m_cameraPosition);
    const ysVector cameraTarget =
        ysMath::Add(
                ysMath::LoadVector(0.0f, 0.0f, 0.0f, 1.0f),
                m_cameraPosition);
    const ysVector up = ysMath::LoadVector(0.0f, 1.0f);

    m_screenVariables.CameraView =
        ysMath::Transpose(ysMath::CameraTarget(cameraEye, cameraTarget, up));
    m_screenVariables.Eye = ysMath::LoadVector(cameraEye);
}

void Shaders::CalculateUiCamera(float screenWidth, float screenHeight) {
    m_uiScreenVariables.Projection = ysMath::Transpose(
        ysMath::OrthographicProjection(
            screenWidth,
            screenHeight,
            0.001f,
            500.0f));

    const ysVector cameraEye =
        ysMath::LoadVector(0.0f, 0.0f, 10.0f, 1.0f);
    const ysVector cameraTarget =
        ysMath::LoadVector(0.0f, 0.0f, 0.0f, 1.0f);
    const ysVector up = ysMath::LoadVector(0.0f, 1.0f);

    m_uiScreenVariables.CameraView =
        ysMath::Transpose(ysMath::CameraTarget(cameraEye, cameraTarget, up));
    m_uiScreenVariables.Eye = ysMath::LoadVector(cameraEye);
}

void Shaders::SetClearColor(const ysVector &col) {
    m_mainStage->SetClearColor(col);
}
