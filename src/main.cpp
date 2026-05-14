#include "Application.h"
#include "LaunchSettings.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string>

int main(int argc, char* argv[]) {
    std::string modelPath = (argc > 1) ? argv[1] : (ASSETS_DIR "/model.obj");

    LaunchConfig cfg = showLaunchSettings();
    if (!cfg.accepted) return EXIT_SUCCESS;

    try {
        VulkanApplication app;
        app.run(modelPath, cfg);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
