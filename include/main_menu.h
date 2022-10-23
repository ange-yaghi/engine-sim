#ifndef ATG_ENGINE_SIM_MAIN_MENU_H
#define ATG_ENGINE_SIM_MAIN_MENU_H

#include "delta.h"
#include "../include/ui_button.h"
#include <filesystem>

class EngineSimApplication;
enum class MainMenuSceneType {
    MainMenu,
    Change,
    Set
};

class MainMenu {
public:
    MainMenu();
    ~MainMenu();

    static MainMenu Create(EngineSimApplication *app);

public:
    void SetApp(EngineSimApplication* app);

public:
    void render();
    void process();

    void setTransform(float scale, float lx, float ly, float angle);
    void resetShaders();

    void getEngines();
    bool checkExists(int index);
    void setEngine(int index);

protected:
    EngineSimApplication *m_app;
    dbasic::DeltaEngine *m_engine;
    MainMenuSceneType m_sceneType = MainMenuSceneType::MainMenu;

    std::map<std::pair<std::string, bool>, std::pair<std::string, std::filesystem::path>> m_engines;
    std::vector<std::string> m_man;
    int m_selectedMan = 0;
    int m_manCount = 0;

    int m_changePage = 0;
    int m_setPage = 0;

    int m_selectedEngine = 0;
};

#endif /* ATG_ENGINE_SIM_MAIN_MENU_H */
