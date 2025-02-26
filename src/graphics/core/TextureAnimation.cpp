#include "TextureAnimation.hpp"

#ifdef USE_DIRECTX
#include "../../directx/graphics/DXTexture.hpp"
#include "../../directx/window/DXDevice.hpp"
#elif USE_OPENGL
#include "Texture.hpp"
#include "Framebuffer.hpp"
#include <GL/glew.h>
#endif // USE_DIRECTX

#include <unordered_set>

TextureAnimator::TextureAnimator() {
#ifdef USE_DIRECTX
#elif USE_OPENGL
    glGenFramebuffers(1, &fboR);
    glGenFramebuffers(1, &fboD);
#endif // USE_DIRECTX
}

TextureAnimator::~TextureAnimator() {
#ifdef USE_DIRECTX
#elif USE_OPENGL
    glDeleteFramebuffers(1, &fboR);
    glDeleteFramebuffers(1, &fboD);
#endif // USE_DIRECTX
}

void TextureAnimator::addAnimations(const std::vector<TextureAnimation>& animations) {
    for (const auto& elem : animations) {
        addAnimation(elem);
    }
}

void TextureAnimator::update(float delta) {
#ifdef USE_DIRECTX
    std::unordered_set<ID3D11ShaderResourceView*> changedTextures;
    ID3D11DeviceContext* const context = DXDevice::getContext();
#elif USE_OPENGL
    std::unordered_set<uint> changedTextures;
#endif // USE_DIRECTX

    for (auto& elem : animations) {
        elem.timer += delta;
        size_t frameNum = elem.currentFrame;
        Frame frame = elem.frames[elem.currentFrame];
        while (elem.timer >= frame.duration) {
            elem.timer -= frame.duration;
            elem.currentFrame++;
            if (elem.currentFrame >= elem.frames.size()) elem.currentFrame = 0;
            frame = elem.frames[elem.currentFrame];
        }
        if (frameNum != elem.currentFrame){
            auto elemDstId = elem.dstTexture->getId();
            auto elemSrcId = elem.srcTexture->getId();

#ifdef USE_DIRECTX
            changedTextures.insert(elem.dstTexture->getResourceView());
#elif USE_OPENGL
            changedTextures.insert(elemDstId);
            glBindFramebuffer(GL_FRAMEBUFFER, fboD);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, elemDstId, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, fboR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, elemSrcId, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboD);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fboR);
#endif // USE_DIRECTX

            float srcPosY = elem.srcTexture->getHeight() - frame.size.y - frame.srcPos.y; // vertical flip

#ifdef USE_DIRECTX
            CD3D11_BOX box(frame.srcPos.x, srcPosY, 0, frame.srcPos.x + frame.size.x, srcPosY + frame.size.y , 1);
            context->CopySubresourceRegion(elemDstId, 0, frame.dstPos.x, frame.dstPos.y, 0, elemSrcId, 0, &box);

#elif USE_OPENGL
            glBlitFramebuffer(
                frame.srcPos.x, srcPosY,
                frame.srcPos.x + frame.size.x,    
                srcPosY + frame.size.y,
                frame.dstPos.x, frame.dstPos.y,    
                frame.dstPos.x + frame.size.x,
                frame.dstPos.y + frame.size.y,
                GL_COLOR_BUFFER_BIT, GL_NEAREST
            );
#endif // USE_DIRECTX
        }
#ifdef USE_DIRECTX
#elif USE_OPENGL
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
#endif // USE_DIRECTX
    }
#ifdef USE_DIRECTX
    for (auto& elem : changedTextures) {
        context->GenerateMips(elem);
    }
#elif USE_OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    for (auto& elem : changedTextures) {
        glBindTexture(GL_TEXTURE_2D, elem);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
#endif // USE_DIRECTX
}
