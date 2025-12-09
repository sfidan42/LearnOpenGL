#pragma once
#include "Texture.hpp"
#include "Vertex.hpp"
#include <glm/glm.hpp>

struct InstanceData
{
	glm::mat4 model;
    // Add more per-instance attributes here if needed
};

class Model
{
public:
    Model();
    ~Model();

    bool loadGeometry();
    bool loadTexture(const std::string& imagePath);

    bool bind() const;
    void drawInstanced(const std::vector<InstanceData>& instances) const;
    void draw() const;

private:
    std::vector<Vertex> vertices;
    Texture texture{};
    GLuint VAO;
    GLuint VBO;
    GLuint instanceVBO;
};
