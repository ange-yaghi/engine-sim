#ifndef ENGINE_SIM_ENGINE_SIM_APPLICATION_H
#define ENGINE_SIM_ENGINE_SIM_APPLICATION_H

#include "geometry_generator.h"

#include "delta.h"

class EngineSimApplication {
    public:
        EngineSimApplication();
        virtual ~EngineSimApplication();

        void initialize(void *instance, ysContextObject::DeviceAPI api);
        void run();
        void destroy();

        void setCameraPosition(const ysVector &position) { m_cameraPosition = position; }
        void setCameraTarget(const ysVector &target) { m_cameraTarget = target; }
        void setCameraUp(const ysVector &up) { m_cameraUp = up; }

        void drawGenerated(const GeometryGenerator::GeometryIndices &indices);

    protected:
        void renderScene();

    protected:
        virtual void initialize();
        virtual void process(float dt);
        virtual void render();

        dbasic::ShaderSet m_shaderSet;
        dbasic::DefaultShaders m_shaders;

        dbasic::DeltaEngine m_engine;
        dbasic::AssetManager m_assetManager;

        ysVector m_cameraPosition;
        ysVector m_cameraTarget;
        ysVector m_cameraUp;

        std::string m_assetPath;

        ysGPUBuffer *m_geometryVertexBuffer;
        ysGPUBuffer *m_geometryIndexBuffer;

        GeometryGenerator m_geometryGenerator;
};

#endif /* ENGINE_SIM_ENGINE_SIM_APPLICATION_H */
