
#include "../include/engine_sim_application.h"
#include "../include/constants.h"
#include "../include/units.h"

namespace fs = std::filesystem;

MainMenu::MainMenu() {
	m_app = nullptr;
	m_engine = nullptr;
}

MainMenu::~MainMenu() {
	/* void */
}

void MainMenu::SetApp(EngineSimApplication *app) {
	m_app = app;
	m_engine = m_app->getEngine();
}

MainMenu MainMenu::Create(EngineSimApplication *app) {
	MainMenu menu = MainMenu();
	menu.SetApp(app);
	return menu;
}

void MainMenu::setTransform(
	float scale,
	float lx,
	float ly,
	float angle)
{
	const ysMatrix rot = ysMath::RotationTransform(
		ysMath::Constants::ZAxis,
		angle);
	const ysMatrix trans = ysMath::TranslationTransform(
		ysMath::LoadVector((float)lx, (float)ly, 0.0f));
	const ysMatrix scaleTransform = ysMath::ScaleTransform(ysMath::LoadScalar(scale));

	m_app->getShaders()->SetObjectTransform(
		ysMath::MatMult(ysMath::MatMult(trans, rot), scaleTransform));
}

void MainMenu::resetShaders() {
	m_app->getShaders()->ResetBaseColor();
	m_app->getShaders()->SetObjectTransform(ysMath::LoadIdentity());
}

void MainMenu::getEngines() {
	std::map<std::string, std::pair<std::string, std::filesystem::path>> engines;
	m_manCount = 0;
	m_man.clear();

	for (const auto& entry : fs::recursive_directory_iterator("../assets/engines")) {
		if (!entry.is_directory()) {
			std::string ext = entry.path().extension().generic_string();
			if (ext == ".mr") {
				// engine file probably
				std::ifstream file(entry.path().generic_string());
				std::string line;
				std::string lines[2048];
				int i = 0;

				while (std::getline(file, line))
				{
					lines[i] = line;
					i++;
				}

				std::string nodeName = "";
				for (int e = 0; e < i; e++)
				{
					if (lines[e]._Starts_with("public node")) {
						if (lines[e + 1].find("alias output __out: engine;") != std::string::npos) {
							// engine node thing
							lines[e].erase(remove_if(lines[e].begin(), lines[e].end(), isspace), lines[e].end());
							nodeName = lines[e].replace(0, 10, "").replace(lines[e].end()-1, lines[e].end(), "");
						}
					}
				}

				engines.insert(std::make_pair(nodeName, std::make_pair(entry.path().parent_path().generic_string().replace(0, 18, ""), entry.path())));
				file.close();
			}
		}
		else {
			m_manCount++;
			m_man.push_back(entry.path().generic_string().replace(0, 18, ""));
		}
	}

	m_engines.clear();
	m_engines = engines;
}

bool MainMenu::checkExists(int index) {
	int i = 0;

	for each (auto var in m_engines) {
		if (var.second.first == m_man[m_selectedMan]) {
			if (i == index) {
				return true;
			}

			i++;
		}
	}

	return false;
}

void MainMenu::setEngine(int index) {
	int i = 0;

	for each (auto var in m_engines) {
		if (var.second.first == m_man[m_selectedMan]) {
			if (i == index) {
				std::ofstream file("../assets/main.mr", std::ofstream::trunc);

				file << "import \"engine_sim.mr\"" << std::endl;
				file << "import \"themes/default.mr\"" << std::endl;
				file << "import \"" << var.second.second.relative_path().string().replace(0, 10, "") << "\"" << std::endl;
				file << std::endl;
				file << "use_default_theme()" << std::endl;
				file << "main()" << std::endl;

				file.close();
			}

			i++;
		}
	}

	m_app->loadScript();
}

void MainMenu::process() {
	m_engine->GetMousePos(&m_mouseX, &m_mouseY);
	
	if (m_sceneType == MainMenuSceneType::MainMenu) {
		if (m_engine->ProcessKeyDown(ysKey::Code::N1)) {
			m_app->setSceneType(SceneType::Engine);
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::N2)) {
			m_sceneType = MainMenuSceneType::Change;
			getEngines();
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::N3)) {
			//show help
			ShellExecute(NULL, NULL, "https://github.com/ange-yaghi/engine-sim/wiki", NULL, NULL, SW_SHOWNORMAL);
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::N4)) {
			m_app->quit();
		}
	}
	else if(m_sceneType == MainMenuSceneType::Change) {
		{
			int pageOffset = m_changePage * 7;

			if (m_engine->ProcessKeyDown(ysKey::Code::N1)) {
				m_selectedMan = 0 + pageOffset;
				if (m_selectedMan < m_manCount) { m_sceneType = MainMenuSceneType::Set; m_setPage = 0; }
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N2)) {
				m_selectedMan = 1 + pageOffset;
				if (m_selectedMan < m_manCount) { m_sceneType = MainMenuSceneType::Set; m_setPage = 0; }
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N3)) {
				m_selectedMan = 2 + pageOffset;
				if (m_selectedMan < m_manCount) { m_sceneType = MainMenuSceneType::Set; m_setPage = 0; }
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N4)) {
				m_selectedMan = 3 + pageOffset;
				if (m_selectedMan < m_manCount) { m_sceneType = MainMenuSceneType::Set; m_setPage = 0; }
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N5)) {
				m_selectedMan = 4 + pageOffset;
				if (m_selectedMan < m_manCount) { m_sceneType = MainMenuSceneType::Set; m_setPage = 0; }
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N6)) {
				m_selectedMan = 5 + pageOffset;
				if (m_selectedMan < m_manCount) { m_sceneType = MainMenuSceneType::Set; m_setPage = 0; }
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N7)) {
				m_selectedMan = 6 + pageOffset;
				if (m_selectedMan < m_manCount) { m_sceneType = MainMenuSceneType::Set; m_setPage = 0; }
			}
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::Right)) {
			if (m_manCount > 7) {
				if (m_manCount - ((m_changePage+1) * 7) >= 1) {
					m_changePage++;
				}
			}
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::Left)) {
			if (m_manCount > 7) {
				m_changePage--;
				if (m_changePage < 0) {
					m_changePage = 0;
				}
			}
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::N9)) {
			m_app->loadScript();
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::N0)) {
			m_sceneType = MainMenuSceneType::MainMenu;
		}
	}
	else {
		{
			int pageOffset = m_setPage * 7;

			if (m_engine->ProcessKeyDown(ysKey::Code::N1)) {
				m_selectedEngine = 0 + pageOffset;
				if (checkExists(m_selectedEngine)) {
					setEngine(m_selectedEngine);
				}
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N2)) {
				m_selectedEngine = 1 + pageOffset;
				if (checkExists(m_selectedEngine)) {
					setEngine(m_selectedEngine);
				}
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N3)) {
				m_selectedEngine = 2 + pageOffset;
				if (checkExists(m_selectedEngine)) {
					setEngine(m_selectedEngine);
				}
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N4)) {
				m_selectedEngine = 3 + pageOffset;
				if (checkExists(m_selectedEngine)) {
					setEngine(m_selectedEngine);
				}
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N5)) {
				m_selectedEngine = 4 + pageOffset;
				if (checkExists(m_selectedEngine)) {
					setEngine(m_selectedEngine);
				}
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N6)) {
				m_selectedEngine = 5 + pageOffset;
				if (checkExists(m_selectedEngine)) {
					setEngine(m_selectedEngine);
				}
			}
			if (m_engine->ProcessKeyDown(ysKey::Code::N7)) {
				m_selectedEngine = 6 + pageOffset;
				if (checkExists(m_selectedEngine)) {
					setEngine(m_selectedEngine);
				}
			}
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::Right)) {
			if (m_engines.size() > 7) {
				if (m_engines.size() - ((m_setPage + 1) * 7) >= 1) {
					m_setPage++;
				}
			}
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::Left)) {
			if (m_engines.size() > 7) {
				m_setPage--;
				if (m_setPage < 0) {
					m_setPage = 0;
				}
			}
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::N9)) {
			m_app->loadScript();
		}

		if (m_engine->ProcessKeyDown(ysKey::Code::N0)) {
			m_sceneType = MainMenuSceneType::Change;
		}
	}
}

void MainMenu::render() {
	float dt = m_app->getEngine()->GetFrameLength();

	resetShaders();

	m_app->getTextRenderer()->RenderText("ENGINE SIMULATOR", 0 - m_app->getTextRenderer()->CalculateWidth("ENGINE SIMULATOR", 50) / 2, 300, 50);
	m_app->getTextRenderer()->RenderText("ANGETHEGREAT", 0 - m_app->getTextRenderer()->CalculateWidth("ANGETHEGREAT", 40) / 2, 250, 40);

	if (m_sceneType == MainMenuSceneType::MainMenu) {
		m_app->getTextRenderer()->RenderText("PLAY", -300, 100, 40);
		m_app->getTextRenderer()->RenderText("CHANGE ENGINE", -300, 0, 40);
		m_app->getTextRenderer()->RenderText("HELP", -300, -100, 40);
		m_app->getTextRenderer()->RenderText("QUIT", -300, -200, 40);
		
		m_app->getTextRenderer()->RenderText("(1)", 300 - m_app->getTextRenderer()->CalculateWidth("(1)", 40) / 2, 100, 40);
		m_app->getTextRenderer()->RenderText("(2)", 300 - m_app->getTextRenderer()->CalculateWidth("(2)", 40) / 2, 0, 40);
		m_app->getTextRenderer()->RenderText("(3)", 300 - m_app->getTextRenderer()->CalculateWidth("(3)", 40) / 2, -100, 40);
		m_app->getTextRenderer()->RenderText("(4)", 300 - m_app->getTextRenderer()->CalculateWidth("(4)", 40) / 2, -200, 40);
		
		resetShaders();
		setTransform(0.1, -0.15, 0.35, 0);
	}
	else if(m_sceneType == MainMenuSceneType::Change) {
		int max = m_manCount > 7 ? 7 + (m_changePage * 7) : m_manCount;
		for (int i = m_changePage * 7; i < max; i++) {
			if (i < m_manCount) {
				std::string text1 = m_man[i];
				std::string text2 = "(" + std::to_string((i + 1) - m_changePage * 7) + ")";

				if (text1.length() > 26) {
					text1.resize(26);
					text1 += "...";
				}
				m_app->getTextRenderer()->RenderText(text1, -300, 150 - ((i - (m_changePage * 7)) * 50), 30);
				m_app->getTextRenderer()->RenderText(text2, 300 - m_app->getTextRenderer()->CalculateWidth(text2, 20) / 2, 150 - ((i - (m_changePage * 7)) * 50), 30);
			}
		}
		m_app->getTextRenderer()->RenderText(std::to_string(m_changePage), 0 - m_app->getTextRenderer()->CalculateWidth(std::to_string(m_changePage), 20) / 2, -200, 40);

		m_app->getTextRenderer()->RenderText("NEXT (RIGHT)", 330 - m_app->getTextRenderer()->CalculateWidth("NEXT (RIGHT)", 20), -200, 20);
		m_app->getTextRenderer()->RenderText("(LEFT) PREV", -300, -200, 20);

		m_app->getTextRenderer()->RenderText("RELOAD ENGINE", -300, -250, 40);
		m_app->getTextRenderer()->RenderText("(9)", 300 - m_app->getTextRenderer()->CalculateWidth("(9)", 40) / 2, -250, 40);

		m_app->getTextRenderer()->RenderText("BACK", -300, -300, 40);
		m_app->getTextRenderer()->RenderText("(0)", 300 - m_app->getTextRenderer()->CalculateWidth("(0)", 40) / 2, -300, 40);
	}
	else {
		int start = m_setPage * 7;
		int i = 0;
		for each (auto var in m_engines) {
			if (var.second.first == m_man[m_selectedMan]) {
				if (i >= start) {
					if (i - start >= 7) {
						break;
					}

					std::string text1 = var.first;
					std::string text2 = "(" + std::to_string((i + 1) - m_setPage * 7) + ")";

					if (text1.length() > 26) {
						text1.resize(26);
						text1 += "...";
					}
					m_app->getTextRenderer()->RenderText(text1, -300, 150 - ((i - (m_setPage * 7)) * 50), 30);
					m_app->getTextRenderer()->RenderText(text2, 300 - m_app->getTextRenderer()->CalculateWidth(text2, 20) / 2, 150 - ((i - (m_setPage * 7)) * 50), 30);
				}

				i++;
			}
		}

		m_app->getTextRenderer()->RenderText(std::to_string(m_setPage), 0 - m_app->getTextRenderer()->CalculateWidth(std::to_string(m_setPage), 20) / 2, -200, 40);

		m_app->getTextRenderer()->RenderText("NEXT (RIGHT)", 330 - m_app->getTextRenderer()->CalculateWidth("NEXT (RIGHT)", 20), -200, 20);
		m_app->getTextRenderer()->RenderText("(LEFT) PREV", -300, -200, 20);

		m_app->getTextRenderer()->RenderText("RELOAD ENGINE", -300, -250, 40);
		m_app->getTextRenderer()->RenderText("(9)", 300 - m_app->getTextRenderer()->CalculateWidth("(9)", 40) / 2, -250, 40);
		
		m_app->getTextRenderer()->RenderText("BACK", -300, -300, 40);
		m_app->getTextRenderer()->RenderText("(0)", 300 - m_app->getTextRenderer()->CalculateWidth("(0)", 40) / 2, -300, 40);
	}
}
