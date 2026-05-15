#pragma once
#include "Types.h"
#include <string>
#include <vector>

struct Mesh {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    static Mesh loadOBJ(const std::string& path);
    static Mesh makeSphere(int stacks, int sectors);
    static Mesh makeCone(int sectors);
};
