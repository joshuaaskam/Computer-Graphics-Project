//
// Created by Joshua Askam on 6/29/24.
//

#ifndef GRAPHICS_WATERFRAMEBUFFERS_H
#define GRAPHICS_WATERFRAMEBUFFERS_H

#include <glm/ext.hpp>
#include <glad/glad.h>

class WaterFrameBuffers {
public:
    WaterFrameBuffers();

    void cleanUp();
    void bindReflectionFrameBuffer();
    void bindRefractionFrameBuffer();
    void unbindCurrentFrameBuffer();
    uint32_t getReflectionTexture();
    uint32_t getRefractionTexture();
    uint32_t getRefractionDepthTexture();

private:
    static const int REFLECTION_HEIGHT = 400;
    static const int REFRACTION_HEIGHT = 800;

    uint32_t reflectionFrameBuffer;
    uint32_t reflectionTexture;
    uint32_t reflectionDepthBuffer;

    uint32_t refractionFrameBuffer;
    uint32_t refractionTexture;
    uint32_t refractionDepthTexture;

    void initializeReflectionFrameBuffer();
    void initializeRefractionFrameBuffer();
    void bindFrameBuffer( uint32_t frameBuffer, int width, int height);
    uint32_t createFrameBuffer();
    uint32_t createTextureAttachment( int width, int height);
    uint32_t createDepthTextureAttachment( int width, int height);
    uint32_t createDepthBufferAttachment( int width, int height);
protected:
    static const int REFLECTION_WIDTH = 300;
    static const int REFRACTION_WIDTH = 1200;


};


#endif //GRAPHICS_WATERFRAMEBUFFERS_H
