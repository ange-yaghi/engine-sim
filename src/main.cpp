#include "../include/engine_sim_application.h"

#include <iostream>

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    (void)nCmdShow;
    (void)lpCmdLine;
    (void)hPrevInstance;

    EngineSimApplication *app =
        EngineSimApplication::createApplication(
                EngineSimApplication::Application::ArticulatedPendulum); 
    app->initialize((void *)&hInstance, ysContextObject::DeviceAPI::DirectX11); 
    app->run();
    app->destroy();

    return 0;
}
