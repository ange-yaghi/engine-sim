#include "../include/engine_sim_application.h"

#include <iostream>

#include <Windows.h>

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    (void)nCmdShow;
    (void)lpCmdLine;
    (void)hPrevInstance;

    // The Piranha compiler's .mr import search paths (compiler.cpp,
    // "../es/" etc.) are relative to the process's cwd, not to this exe's
    // location -- same issue engine_check_main.cpp works around. Launching
    // engine-sim-app.exe via a shortcut/launcher whose "Start in" isn't the
    // exe's own folder (bin\, per release.bat) leaves cwd wrong and every
    // engine's "import engine_sim.mr" fails to resolve, so no engines load.
    // Pin cwd to the exe's own directory unconditionally so this resolves
    // the same way no matter how the exe was launched.
    char exePath[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, exePath, MAX_PATH) > 0) {
        std::string exeDir = exePath;
        const size_t lastSlash = exeDir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            SetCurrentDirectoryA(exeDir.substr(0, lastSlash).c_str());
        }
    }

    EngineSimApplication application;
    application.initialize((void *)&hInstance, ysContextObject::DeviceAPI::DirectX11);
    application.run();
    application.destroy();

    return 0;
}
