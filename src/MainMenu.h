#pragma once

struct MainMenuResult {
    enum Choice { RUBIKS, SHOOTER, QUIT } choice = QUIT;
    int width = 1280, height = 720, targetFPS = 0;
    bool accepted = false;
};

MainMenuResult showMainMenu();
