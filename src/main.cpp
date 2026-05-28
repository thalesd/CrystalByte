#include "Application.h"
#include "LaunchSettings.h"
#include "MainMenu.h"
#include "RubiksCubeApp.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string>

int main(int argc, char* argv[]) {
    std::string modelPath = (argc > 1) ? argv[1] : (ASSETS_DIR "/model.obj");

    MainMenuResult menu = showMainMenu();
    if (!menu.accepted) return EXIT_SUCCESS;

    try {
        if (menu.choice == MainMenuResult::RUBIKS) {
            RubiksCubeApp app;
            app.run(menu);
        } else {
            LaunchConfig cfg;
            cfg.width     = menu.width;
            cfg.height    = menu.height;
            cfg.targetFPS = menu.targetFPS;
            cfg.accepted  = true;
            VulkanApplication app;
            app.run(modelPath, cfg);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
