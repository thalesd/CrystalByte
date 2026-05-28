#pragma once

struct UpgradeState {
    int totalScore      = 0;
    int multiplierLevel = 0; // 0=1x, 1=1.5x, 2=2x, 3=3x
    int undosRemaining  = 0;
};

enum class StoreAction { CONTINUE, NEW_GAME, QUIT };

StoreAction showUpgradeStore(UpgradeState& state, int lastCommitScore);
