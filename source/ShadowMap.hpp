#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.hpp"

using namespace glm;

class ShadowMap
{
public:
	explicit ShadowMap(unsigned int width = 2048, unsigned int height = 2048);
    ~ShadowMap();

    // Non-copyable
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    // Bind framebuffer for shadow pass
    void bindForWriting() const;

    // Bind shadow map texture for reading in main shader
    void bindForReading(GLenum textureUnit) const;

    // Get light space matrix for directional light
	static mat4 getLightSpaceMatrix(const vec3& lightDir, float orthoSize = 20.0f, float nearPlane = 1.0f, float farPlane = 50.0f);

    [[nodiscard]] unsigned int getWidth() const { return shadowWidth; }
    [[nodiscard]] unsigned int getHeight() const { return shadowHeight; }
    [[nodiscard]] GLuint getDepthTexture() const { return depthTexture; }

private:
    GLuint depthMapFBO = 0;
    GLuint depthTexture = 0;
    uint32_t shadowWidth;
    uint32_t shadowHeight;
};
