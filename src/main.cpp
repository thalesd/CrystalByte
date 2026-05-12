#include "Application.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string>

int main(int argc, char* argv[]) {
    std::string modelPath = (argc > 1) ? argv[1] : (ASSETS_DIR "/model.obj");
    try {
        VulkanApplication app;
        app.run(modelPath);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
