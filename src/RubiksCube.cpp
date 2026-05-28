#include "RubiksCube.h"
#include <cstdint>

// ---------------------------------------------------------------------------
// reset
// ---------------------------------------------------------------------------

void RubiksCube::reset() {
    // faces[F_UP]=WHITE, faces[F_DOWN]=YELLOW, faces[F_FRONT]=RED,
    // faces[F_BACK]=ORANGE, faces[F_LEFT]=GREEN, faces[F_RIGHT]=BLUE
    const RColor faceColors[6] = {
        RColor::WHITE,   // F_UP   = 0
        RColor::YELLOW,  // F_DOWN = 1
        RColor::RED,     // F_FRONT= 2
        RColor::ORANGE,  // F_BACK = 3
        RColor::GREEN,   // F_LEFT = 4
        RColor::BLUE     // F_RIGHT= 5
    };
    for (int f = 0; f < 6; ++f)
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                faces[f][r][c] = faceColors[f];
}

// ---------------------------------------------------------------------------
// Face rotation helpers
// ---------------------------------------------------------------------------

void RubiksCube::rotateFaceCW(std::array<std::array<RColor,3>,3>& f) {
    std::array<std::array<RColor,3>,3> tmp = f;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            f[r][c] = tmp[2-c][r];
}

void RubiksCube::rotateFaceCCW(std::array<std::array<RColor,3>,3>& f) {
    std::array<std::array<RColor,3>,3> tmp = f;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            f[r][c] = tmp[c][2-r];
}

// ---------------------------------------------------------------------------
// moveU
// ---------------------------------------------------------------------------

void RubiksCube::moveU(bool cw) {
    if (cw) {
        rotateFaceCW(faces[F_UP]);
        // CW: FRONT[0]->RIGHT[0]->BACK[0]->LEFT[0]->FRONT[0]
        // tmp=FRONT[0], FRONT[0]=LEFT[0], LEFT[0]=BACK[0], BACK[0]=RIGHT[0], RIGHT[0]=tmp
        std::array<RColor,3> tmp = faces[F_FRONT][0];
        faces[F_FRONT][0] = faces[F_LEFT][0];
        faces[F_LEFT][0]  = faces[F_BACK][0];
        faces[F_BACK][0]  = faces[F_RIGHT][0];
        faces[F_RIGHT][0] = tmp;
    } else {
        rotateFaceCCW(faces[F_UP]);
        // CCW: reverse
        std::array<RColor,3> tmp = faces[F_FRONT][0];
        faces[F_FRONT][0] = faces[F_RIGHT][0];
        faces[F_RIGHT][0] = faces[F_BACK][0];
        faces[F_BACK][0]  = faces[F_LEFT][0];
        faces[F_LEFT][0]  = tmp;
    }
}

// ---------------------------------------------------------------------------
// moveD
// ---------------------------------------------------------------------------

void RubiksCube::moveD(bool cw) {
    if (cw) {
        rotateFaceCW(faces[F_DOWN]);
        // CW: FRONT[2]->LEFT[2]->BACK[2]->RIGHT[2]->FRONT[2]
        // tmp=FRONT[2], FRONT[2]=RIGHT[2], RIGHT[2]=BACK[2], BACK[2]=LEFT[2], LEFT[2]=tmp
        std::array<RColor,3> tmp = faces[F_FRONT][2];
        faces[F_FRONT][2] = faces[F_RIGHT][2];
        faces[F_RIGHT][2] = faces[F_BACK][2];
        faces[F_BACK][2]  = faces[F_LEFT][2];
        faces[F_LEFT][2]  = tmp;
    } else {
        rotateFaceCCW(faces[F_DOWN]);
        std::array<RColor,3> tmp = faces[F_FRONT][2];
        faces[F_FRONT][2] = faces[F_LEFT][2];
        faces[F_LEFT][2]  = faces[F_BACK][2];
        faces[F_BACK][2]  = faces[F_RIGHT][2];
        faces[F_RIGHT][2] = tmp;
    }
}

// ---------------------------------------------------------------------------
// moveF
// ---------------------------------------------------------------------------

void RubiksCube::moveF(bool cw) {
    if (cw) {
        rotateFaceCW(faces[F_FRONT]);
        std::array<RColor,3> tmp;
        for (int i = 0; i < 3; ++i) tmp[i] = faces[F_UP][2][i];
        // UP[2][i] = LEFT[2-i][2]
        for (int i = 0; i < 3; ++i) faces[F_UP][2][i] = faces[F_LEFT][2-i][2];
        // LEFT[i][2] = DOWN[0][i]
        for (int i = 0; i < 3; ++i) faces[F_LEFT][i][2] = faces[F_DOWN][0][i];
        // DOWN[0][i] = RIGHT[2-i][0]
        for (int i = 0; i < 3; ++i) faces[F_DOWN][0][i] = faces[F_RIGHT][2-i][0];
        // RIGHT[i][0] = tmp[i]
        for (int i = 0; i < 3; ++i) faces[F_RIGHT][i][0] = tmp[i];
    } else {
        rotateFaceCCW(faces[F_FRONT]);
        std::array<RColor,3> tmp;
        for (int i = 0; i < 3; ++i) tmp[i] = faces[F_UP][2][i];
        // UP[2][i] = RIGHT[i][0]
        for (int i = 0; i < 3; ++i) faces[F_UP][2][i] = faces[F_RIGHT][i][0];
        // RIGHT[i][0] = DOWN[0][2-i]
        for (int i = 0; i < 3; ++i) faces[F_RIGHT][i][0] = faces[F_DOWN][0][2-i];
        // DOWN[0][i] = LEFT[i][2]
        for (int i = 0; i < 3; ++i) faces[F_DOWN][0][i] = faces[F_LEFT][i][2];
        // LEFT[i][2] = tmp[2-i]
        for (int i = 0; i < 3; ++i) faces[F_LEFT][i][2] = tmp[2-i];
    }
}

// ---------------------------------------------------------------------------
// moveB
// ---------------------------------------------------------------------------

void RubiksCube::moveB(bool cw) {
    if (cw) {
        rotateFaceCW(faces[F_BACK]);
        std::array<RColor,3> tmp;
        for (int i = 0; i < 3; ++i) tmp[i] = faces[F_UP][0][i];
        // UP[0][i] = RIGHT[i][2]
        for (int i = 0; i < 3; ++i) faces[F_UP][0][i] = faces[F_RIGHT][i][2];
        // RIGHT[i][2] = DOWN[2][2-i]
        for (int i = 0; i < 3; ++i) faces[F_RIGHT][i][2] = faces[F_DOWN][2][2-i];
        // DOWN[2][i] = LEFT[i][0]
        for (int i = 0; i < 3; ++i) faces[F_DOWN][2][i] = faces[F_LEFT][i][0];
        // LEFT[i][0] = tmp[2-i]
        for (int i = 0; i < 3; ++i) faces[F_LEFT][i][0] = tmp[2-i];
    } else {
        rotateFaceCCW(faces[F_BACK]);
        std::array<RColor,3> tmp;
        for (int i = 0; i < 3; ++i) tmp[i] = faces[F_UP][0][i];
        // UP[0][i] = LEFT[2-i][0]
        for (int i = 0; i < 3; ++i) faces[F_UP][0][i] = faces[F_LEFT][2-i][0];
        // LEFT[i][0] = DOWN[2][i]
        for (int i = 0; i < 3; ++i) faces[F_LEFT][i][0] = faces[F_DOWN][2][i];
        // DOWN[2][i] = RIGHT[2-i][2]
        for (int i = 0; i < 3; ++i) faces[F_DOWN][2][i] = faces[F_RIGHT][2-i][2];
        // RIGHT[i][2] = tmp[i]
        for (int i = 0; i < 3; ++i) faces[F_RIGHT][i][2] = tmp[i];
    }
}

// ---------------------------------------------------------------------------
// moveL
// ---------------------------------------------------------------------------

void RubiksCube::moveL(bool cw) {
    if (cw) {
        rotateFaceCW(faces[F_LEFT]);
        std::array<RColor,3> tmp;
        for (int i = 0; i < 3; ++i) tmp[i] = faces[F_UP][i][0];
        // UP[i][0] = BACK[2-i][2]
        for (int i = 0; i < 3; ++i) faces[F_UP][i][0] = faces[F_BACK][2-i][2];
        // BACK[i][2] = DOWN[2-i][0]
        for (int i = 0; i < 3; ++i) faces[F_BACK][i][2] = faces[F_DOWN][2-i][0];
        // DOWN[i][0] = FRONT[i][0]
        for (int i = 0; i < 3; ++i) faces[F_DOWN][i][0] = faces[F_FRONT][i][0];
        // FRONT[i][0] = tmp[i]
        for (int i = 0; i < 3; ++i) faces[F_FRONT][i][0] = tmp[i];
    } else {
        rotateFaceCCW(faces[F_LEFT]);
        std::array<RColor,3> tmp;
        for (int i = 0; i < 3; ++i) tmp[i] = faces[F_UP][i][0];
        // UP[i][0] = FRONT[i][0]
        for (int i = 0; i < 3; ++i) faces[F_UP][i][0] = faces[F_FRONT][i][0];
        // FRONT[i][0] = DOWN[i][0]
        for (int i = 0; i < 3; ++i) faces[F_FRONT][i][0] = faces[F_DOWN][i][0];
        // DOWN[i][0] = BACK[2-i][2]
        for (int i = 0; i < 3; ++i) faces[F_DOWN][i][0] = faces[F_BACK][2-i][2];
        // BACK[i][2] = tmp[2-i]
        for (int i = 0; i < 3; ++i) faces[F_BACK][i][2] = tmp[2-i];
    }
}

// ---------------------------------------------------------------------------
// moveR
// ---------------------------------------------------------------------------

void RubiksCube::moveR(bool cw) {
    if (cw) {
        rotateFaceCW(faces[F_RIGHT]);
        std::array<RColor,3> tmp;
        for (int i = 0; i < 3; ++i) tmp[i] = faces[F_UP][i][2];
        // UP[i][2] = BACK[2-i][0]
        for (int i = 0; i < 3; ++i) faces[F_UP][i][2] = faces[F_BACK][2-i][0];
        // BACK[i][0] = DOWN[2-i][2]
        for (int i = 0; i < 3; ++i) faces[F_BACK][i][0] = faces[F_DOWN][2-i][2];
        // DOWN[i][2] = FRONT[i][2]
        for (int i = 0; i < 3; ++i) faces[F_DOWN][i][2] = faces[F_FRONT][i][2];
        // FRONT[i][2] = tmp[i]
        for (int i = 0; i < 3; ++i) faces[F_FRONT][i][2] = tmp[i];
    } else {
        rotateFaceCCW(faces[F_RIGHT]);
        std::array<RColor,3> tmp;
        for (int i = 0; i < 3; ++i) tmp[i] = faces[F_UP][i][2];
        // UP[i][2] = FRONT[i][2]
        for (int i = 0; i < 3; ++i) faces[F_UP][i][2] = faces[F_FRONT][i][2];
        // FRONT[i][2] = DOWN[i][2]
        for (int i = 0; i < 3; ++i) faces[F_FRONT][i][2] = faces[F_DOWN][i][2];
        // DOWN[i][2] = BACK[2-i][0]
        for (int i = 0; i < 3; ++i) faces[F_DOWN][i][2] = faces[F_BACK][2-i][0];
        // BACK[i][0] = tmp[2-i]
        for (int i = 0; i < 3; ++i) faces[F_BACK][i][0] = tmp[2-i];
    }
}

// ---------------------------------------------------------------------------
// computeScore
// ---------------------------------------------------------------------------

int RubiksCube::computeScore() const {
    int score = 0;
    for (int f = 0; f < 6; ++f) {
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                // right neighbor
                if (c + 1 < 3 && faces[f][r][c] == faces[f][r][c+1]) ++score;
                // below neighbor
                if (r + 1 < 3 && faces[f][r][c] == faces[f][r+1][c]) ++score;
            }
        }
    }
    return score;
}

// ---------------------------------------------------------------------------
// isSolved
// ---------------------------------------------------------------------------

bool RubiksCube::isSolved() const {
    return computeScore() == 72;
}

// ---------------------------------------------------------------------------
// scramble
// ---------------------------------------------------------------------------

void RubiksCube::scramble(int numMoves) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 11);

    for (int i = 0; i < numMoves; ++i) {
        switch (dist(rng)) {
            case  0: moveL(true);  break;
            case  1: moveL(false); break;
            case  2: moveR(true);  break;
            case  3: moveR(false); break;
            case  4: moveU(true);  break;
            case  5: moveU(false); break;
            case  6: moveD(true);  break;
            case  7: moveD(false); break;
            case  8: moveF(true);  break;
            case  9: moveF(false); break;
            case 10: moveB(true);  break;
            case 11: moveB(false); break;
        }
    }
}

// ---------------------------------------------------------------------------
// applyMoveToCube
// ---------------------------------------------------------------------------

void applyMoveToCube(RubiksCube& cube, const CubeMove& m) {
    if (m.axis == 0) {
        if (m.layer == 0) {
            cube.moveL(m.sign > 0);
        } else {
            cube.moveR(m.sign < 0);
        }
    } else if (m.axis == 1) {
        if (m.layer == 2) {
            cube.moveU(m.sign > 0);
        } else {
            cube.moveD(m.sign < 0);
        }
    } else { // axis == 2
        if (m.layer == 2) {
            cube.moveF(m.sign < 0);
        } else {
            cube.moveB(m.sign > 0);
        }
    }
}
