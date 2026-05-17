#pragma once

struct LaunchConfig {
    int  width     = 1280;
    int  height    = 720;
    int  targetFPS = 0;     // 0 = unlimited
    bool showBvh   = false;
    bool accepted  = false;
};

LaunchConfig showLaunchSettings();
