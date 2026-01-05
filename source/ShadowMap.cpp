#include "ShadowMap.hpp"
#include <iostream>
#include <array>

ShadowMap::ShadowMap(const uint32_t width, const uint32_t height)
    : shadowWidth(width), shadowHeight(height)
{
    // Create framebuffer for shadow map
    glGenFramebuffers(1, &depthMapFBO);

    // Create depth texture
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Set border color to 1.0 (no shadow outside the shadow map)
    const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Enable hardware PCF (Percentage Closer Filtering)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    // No color buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ERROR: Shadow map framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowMap::~ShadowMap()
{
    if (depthTexture)
        glDeleteTextures(1, &depthTexture);
    if (depthMapFBO)
        glDeleteFramebuffers(1, &depthMapFBO);
}

void ShadowMap::bindForWriting() const
{
    glViewport(0, 0, shadowWidth, shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::bindForReading(GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
}

mat4 ShadowMap::getLightSpaceMatrix(const vec3& lightDir, const float orthoSize, const float nearPlane, const float farPlane)
{
    // For directional light, we use orthographic projection
    const mat4 lightProjection = ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);

    // Light view matrix - looking from "light position" towards origin
    // For directional light, position is arbitrary, but far enough to encompass scene
    const vec3 lightPos = -lightDir * 25.0f; // Position light "behind" the direction
    const mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}

// ==================== SpotLightShadowMap ====================

SpotLightShadowMap::SpotLightShadowMap(unsigned int width, unsigned int height)
    : shadowWidth(width), shadowHeight(height)
{
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthTexture);

    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Enable hardware PCF
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR: Spot light shadow map framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

SpotLightShadowMap::~SpotLightShadowMap()
{
    if (depthTexture)
        glDeleteTextures(1, &depthTexture);
    if (depthMapFBO)
        glDeleteFramebuffers(1, &depthMapFBO);
}

SpotLightShadowMap::SpotLightShadowMap(SpotLightShadowMap&& other) noexcept
    : depthMapFBO(other.depthMapFBO), depthTexture(other.depthTexture),
      shadowWidth(other.shadowWidth), shadowHeight(other.shadowHeight)
{
    other.depthMapFBO = 0;
    other.depthTexture = 0;
}

SpotLightShadowMap& SpotLightShadowMap::operator=(SpotLightShadowMap&& other) noexcept
{
    if (this != &other)
    {
        if (depthTexture) glDeleteTextures(1, &depthTexture);
        if (depthMapFBO) glDeleteFramebuffers(1, &depthMapFBO);

        depthMapFBO = other.depthMapFBO;
        depthTexture = other.depthTexture;
        shadowWidth = other.shadowWidth;
        shadowHeight = other.shadowHeight;

        other.depthMapFBO = 0;
        other.depthTexture = 0;
    }
    return *this;
}

void SpotLightShadowMap::bindForWriting() const
{
    glViewport(0, 0, shadowWidth, shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void SpotLightShadowMap::bindForReading(GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
}

mat4 SpotLightShadowMap::getLightSpaceMatrix(const vec3& position, const vec3& direction,
                                               float outerCutOff, float nearPlane, float farPlane) const
{
    // FOV should be at least as wide as the outer cone angle (outerCutOff is cos of angle)
    float fov = acos(outerCutOff) * 2.0f;
    fov = glm::clamp(fov, glm::radians(30.0f), glm::radians(120.0f));

    float aspect = static_cast<float>(shadowWidth) / static_cast<float>(shadowHeight);
    mat4 lightProjection = perspective(fov, aspect, nearPlane, farPlane);

    // Calculate up vector that isn't parallel to direction
    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    if (abs(dot(direction, up)) > 0.99f)
        up = vec3(1.0f, 0.0f, 0.0f);

    mat4 lightView = lookAt(position, position + direction, up);

    return lightProjection * lightView;
}

// ==================== PointLightShadowMap ====================

PointLightShadowMap::PointLightShadowMap(unsigned int size)
    : shadowSize(size)
{
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthCubemap);

    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
                     shadowSize, shadowSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // No hardware comparison for cubemaps - we do manual depth comparison in shader

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR: Point light shadow map framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PointLightShadowMap::~PointLightShadowMap()
{
    if (depthCubemap)
        glDeleteTextures(1, &depthCubemap);
    if (depthMapFBO)
        glDeleteFramebuffers(1, &depthMapFBO);
}

PointLightShadowMap::PointLightShadowMap(PointLightShadowMap&& other) noexcept
    : depthMapFBO(other.depthMapFBO), depthCubemap(other.depthCubemap), shadowSize(other.shadowSize)
{
    other.depthMapFBO = 0;
    other.depthCubemap = 0;
}

PointLightShadowMap& PointLightShadowMap::operator=(PointLightShadowMap&& other) noexcept
{
    if (this != &other)
    {
        if (depthCubemap) glDeleteTextures(1, &depthCubemap);
        if (depthMapFBO) glDeleteFramebuffers(1, &depthMapFBO);

        depthMapFBO = other.depthMapFBO;
        depthCubemap = other.depthCubemap;
        shadowSize = other.shadowSize;

        other.depthMapFBO = 0;
        other.depthCubemap = 0;
    }
    return *this;
}

void PointLightShadowMap::bindForWriting() const
{
    glViewport(0, 0, shadowSize, shadowSize);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void PointLightShadowMap::bindForReading(GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
}

std::array<mat4, 6> PointLightShadowMap::getLightViewMatrices(const vec3& lightPos)
{
    return {
        lookAt(lightPos, lightPos + vec3( 1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)), // +X
        lookAt(lightPos, lightPos + vec3(-1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)), // -X
        lookAt(lightPos, lightPos + vec3( 0.0f,  1.0f,  0.0f), vec3(0.0f,  0.0f,  1.0f)), // +Y
        lookAt(lightPos, lightPos + vec3( 0.0f, -1.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f)), // -Y
        lookAt(lightPos, lightPos + vec3( 0.0f,  0.0f,  1.0f), vec3(0.0f, -1.0f,  0.0f)), // +Z
        lookAt(lightPos, lightPos + vec3( 0.0f,  0.0f, -1.0f), vec3(0.0f, -1.0f,  0.0f))  // -Z
    };
}

mat4 PointLightShadowMap::getLightProjectionMatrix(float nearPlane, float farPlane) const
{
    // 90 degree FOV for cubemap face, aspect ratio 1:1
    return perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
}
