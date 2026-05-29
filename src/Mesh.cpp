#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>


Mesh Mesh::makeSphere(int stacks, int sectors) {
    Mesh mesh;
    const float pi = glm::pi<float>();

    for (int i = 0; i <= stacks; ++i) {
        float phi = pi * static_cast<float>(i) / static_cast<float>(stacks);
        for (int j = 0; j <= sectors; ++j) {
            float theta = 2.0f * pi * static_cast<float>(j) / static_cast<float>(sectors);

            Vertex v{};
            v.pos.x    = std::sin(phi) * std::cos(theta);
            v.pos.y    = std::cos(phi);
            v.pos.z    = std::sin(phi) * std::sin(theta);
            v.normal   = v.pos; // unit sphere: normal == position
            v.texCoord = { static_cast<float>(j) / sectors,
                           static_cast<float>(i) / stacks };
            mesh.vertices.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (sectors + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(sectors + 1);
            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(a + 1);
            mesh.indices.push_back(b);
            mesh.indices.push_back(b + 1);
            mesh.indices.push_back(a + 1);
        }
    }

    return mesh;
}

Mesh Mesh::makeCone(int sectors) {
    Mesh mesh;
    const float pi = glm::pi<float>();

    // Lateral surface — one triangle per sector: apex + two base edge vertices.
    // Analytic outward normal for the unit cone (apex y=+1, base radius 1 at y=-1):
    //   n(theta) = normalize(2*sin(theta), 1, 2*cos(theta))
    for (int j = 0; j < sectors; ++j) {
        float t0 = 2.0f * pi * static_cast<float>(j)     / sectors;
        float t1 = 2.0f * pi * static_cast<float>(j + 1) / sectors;
        float tM = (t0 + t1) * 0.5f;

        Vertex a{}; // apex — midpoint normal for smooth shading at the tip
        a.pos      = { 0.0f, 1.0f, 0.0f };
        a.normal   = glm::normalize(glm::vec3(2.0f * std::sin(tM), 1.0f, 2.0f * std::cos(tM)));
        a.texCoord = { (static_cast<float>(j) + 0.5f) / sectors, 0.0f };

        Vertex b{};
        b.pos      = { std::sin(t0), -1.0f, std::cos(t0) };
        b.normal   = glm::normalize(glm::vec3(2.0f * std::sin(t0), 1.0f, 2.0f * std::cos(t0)));
        b.texCoord = { static_cast<float>(j)     / sectors, 1.0f };

        Vertex c{};
        c.pos      = { std::sin(t1), -1.0f, std::cos(t1) };
        c.normal   = glm::normalize(glm::vec3(2.0f * std::sin(t1), 1.0f, 2.0f * std::cos(t1)));
        c.texCoord = { static_cast<float>(j + 1) / sectors, 1.0f };

        uint32_t idx = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(a);
        mesh.vertices.push_back(b);
        mesh.vertices.push_back(c);
        mesh.indices.push_back(idx);
        mesh.indices.push_back(idx + 1);
        mesh.indices.push_back(idx + 2);
    }

    // Bottom cap — flat disk, normal pointing down (0,-1,0).
    // CCW winding viewed from below: center → c → b.
    for (int j = 0; j < sectors; ++j) {
        float t0 = 2.0f * pi * static_cast<float>(j)     / sectors;
        float t1 = 2.0f * pi * static_cast<float>(j + 1) / sectors;

        const glm::vec3 dn = { 0.0f, -1.0f, 0.0f };

        Vertex center{};
        center.pos      = { 0.0f, -1.0f, 0.0f };
        center.normal   = dn;
        center.texCoord = { 0.5f, 0.5f };

        Vertex b{};
        b.pos      = { std::sin(t0), -1.0f, std::cos(t0) };
        b.normal   = dn;
        b.texCoord = { 0.5f + 0.5f * std::sin(t0), 0.5f + 0.5f * std::cos(t0) };

        Vertex c{};
        c.pos      = { std::sin(t1), -1.0f, std::cos(t1) };
        c.normal   = dn;
        c.texCoord = { 0.5f + 0.5f * std::sin(t1), 0.5f + 0.5f * std::cos(t1) };

        uint32_t idx = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(center);
        mesh.vertices.push_back(b);
        mesh.vertices.push_back(c);
        mesh.indices.push_back(idx);
        mesh.indices.push_back(idx + 2);
        mesh.indices.push_back(idx + 1);
    }

    return mesh;
}

Mesh Mesh::makeCube() {
    // Unit cube centered at origin. Each face has 4 vertices with a per-face normal
    // so that flat shading is correct. CCW winding viewed from outside (back-face cull).
    Mesh mesh;

    struct FaceDef { glm::vec3 n; glm::vec3 v[4]; };
    const FaceDef faces[6] = {
        { { 1, 0, 0}, {{ 0.5f,-0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f, 0.5f},{ 0.5f,-0.5f, 0.5f}} },
        { {-1, 0, 0}, {{-0.5f,-0.5f, 0.5f},{-0.5f, 0.5f, 0.5f},{-0.5f, 0.5f,-0.5f},{-0.5f,-0.5f,-0.5f}} },
        { { 0, 1, 0}, {{ 0.5f, 0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},{-0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f}} },
        { { 0,-1, 0}, {{-0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f, 0.5f},{-0.5f,-0.5f, 0.5f}} },
        { { 0, 0, 1}, {{-0.5f,-0.5f, 0.5f},{ 0.5f,-0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},{-0.5f, 0.5f, 0.5f}} },
        { { 0, 0,-1}, {{ 0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f}} }
    };

    for (const auto& f : faces) {
        auto base = static_cast<uint32_t>(mesh.vertices.size());
        for (int i = 0; i < 4; ++i) {
            Vertex v{};
            v.pos      = f.v[i];
            v.normal   = f.n;
            v.texCoord = {};
            mesh.vertices.push_back(v);
        }
        mesh.indices.insert(mesh.indices.end(),
            { base, base+1, base+2, base, base+2, base+3 });
    }
    return mesh;
}
