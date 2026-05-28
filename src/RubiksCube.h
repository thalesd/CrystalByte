#pragma once
#include <array>
#include <vector>
#include <random>

enum class RColor : uint8_t { WHITE=0, YELLOW=1, RED=2, ORANGE=3, BLUE=4, GREEN=5 };

// Face indices
constexpr int F_UP=0, F_DOWN=1, F_FRONT=2, F_BACK=3, F_LEFT=4, F_RIGHT=5;

struct RubiksCube {
    // faces[faceId][row][col], row 0=top col 0=left when looking at face from outside
    std::array<std::array<std::array<RColor,3>,3>,6> faces;

    void reset();
    void scramble(int numMoves = 25);

    void moveU(bool cw = true);
    void moveD(bool cw = true);
    void moveF(bool cw = true);
    void moveB(bool cw = true);
    void moveL(bool cw = true);
    void moveR(bool cw = true);

    int  computeScore() const;
    bool isSolved() const;

private:
    static void rotateFaceCW (std::array<std::array<RColor,3>,3>& f);
    static void rotateFaceCCW(std::array<std::array<RColor,3>,3>& f);
};

struct CubeMove {
    int   axis;  // 0=X 1=Y 2=Z
    int   layer; // 0 or 2
    float sign;  // +1 or -1 rotation direction

    static CubeMove L()  { return {0,0,+1}; }
    static CubeMove LP() { return {0,0,-1}; }
    static CubeMove R()  { return {0,2,-1}; }
    static CubeMove RP() { return {0,2,+1}; }
    static CubeMove U()  { return {1,2,+1}; }
    static CubeMove UP() { return {1,2,-1}; }
    static CubeMove D()  { return {1,0,-1}; }
    static CubeMove DP() { return {1,0,+1}; }
    static CubeMove F()  { return {2,2,-1}; }
    static CubeMove FP() { return {2,2,+1}; }
    static CubeMove B()  { return {2,0,+1}; }
    static CubeMove BP() { return {2,0,-1}; }
};

void applyMoveToCube(RubiksCube& cube, const CubeMove& m);
