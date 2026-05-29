#include "RubiksCubeApp.h"
#include "MainMenu.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main() {
    MainMenuResult menu = showMainMenu();
    if (!menu.accepted) return EXIT_SUCCESS;

    try {
        RubiksCubeApp app;
        app.run(menu);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
