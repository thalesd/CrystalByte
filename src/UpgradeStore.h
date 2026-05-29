#pragma once
#include "RubiksCube.h"
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Upgrade types
// ---------------------------------------------------------------------------

enum class UpgradeType {
    SCORE_MULTIPLIER,   // global commit multiplier +0.5x per level (max 3)
    UNDO_PACK,          // +5 undo moves
    COLOR_BONUS,        // one color scores 2x per adjacent pair
    FAST_MOVES,         // halve animation duration
    INSTANT_POINTS,     // immediately gain 15 pts (free)
};

struct UpgradeOption {
    UpgradeType type      = UpgradeType::INSTANT_POINTS;
    int         cost      = 0;
    int         colorIdx  = 0;   // only used for COLOR_BONUS
    std::string label;
};

// ---------------------------------------------------------------------------
// Persistent state carried between commits
// ---------------------------------------------------------------------------

struct UpgradeState {
    int  totalScore      = 0;
    int  multiplierLevel = 0;       // 0=1x, 1=1.5x, 2=2x, 3=2.5x
    int  undosRemaining  = 0;
    bool colorBonus[6]   = {};      // per-RColor bonus: true = that color scores 2x
    bool fastMoves       = false;   // animation at half duration
};

// ---------------------------------------------------------------------------
// Store interface
// ---------------------------------------------------------------------------

enum class StoreAction { CONTINUE, QUIT };

// Generate 3 upgrade options appropriate for the current state
std::vector<UpgradeOption> generateUpgradeOptions(const UpgradeState& state);

// Show the upgrade store dialog; upgrades is modified in-place when bought
StoreAction showUpgradeStore(UpgradeState& state, int lastCommitScore,
                             const std::vector<UpgradeOption>& options);
