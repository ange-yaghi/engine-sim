#include "../include/engine_sim_application.h"

static void runApp(void *handle) {
    EngineSimApplication application;
    application.initialize(handle, ysContextObject::DeviceAPI::OpenGL4_0);
    application.run();
    application.destroy();
}

#if _WIN32
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    (void)nCmdShow;
    (void)lpCmdLine;
    (void)hPrevInstance;

    runApp(static_cast<void*>(&hInstance));

    return 0;
}

#else
int main() {
    runApp(nullptr);
    return 0;
}

#endif
