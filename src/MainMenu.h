#pragma once

struct MainMenuResult {
    int  width     = 1280;
    int  height    = 720;
    int  targetFPS = 0;
    bool accepted  = false;
};

MainMenuResult showMainMenu();
