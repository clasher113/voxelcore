#include "PostProcessing.hpp"

#include "Viewport.hpp"
#include "DrawContext.hpp"

#ifdef USE_DIRECTX
#include "../../directx/graphics/DXMesh.hpp"
#include "../../directx/graphics/DXShader.hpp"
#include "../../directx/graphics/DXTexture.hpp"
#include "../../directx/graphics/DXFramebuffer.hpp"
#elif USE_OPENGL
#include "Mesh.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Framebuffer.hpp"
#endif // USE_DIRECTX

#include <stdexcept>

PostProcessing::PostProcessing() {
    // Fullscreen quad mesh bulding

    float vertices[]{
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
    };

    vattr attrs[] {{2}, {0}};
    quadMesh = std::make_unique<Mesh>(vertices, 6, attrs);
}

PostProcessing::~PostProcessing() = default;

void PostProcessing::use(DrawContext& context) {
    const auto& vp = context.getViewport();
    if (fbo) {
        fbo->resize(vp.getWidth(), vp.getHeight());
    } else {
        fbo = std::make_unique<Framebuffer>(vp.getWidth(), vp.getHeight());
    }
    context.setFramebuffer(fbo.get());
}

void PostProcessing::render(const DrawContext& context, Shader* screenShader) {
    if (fbo == nullptr) {
        throw std::runtime_error("'use(...)' was never called");
    }

    const auto& viewport = context.getViewport();
    screenShader->use();
    screenShader->uniform2i("u_screenSize", viewport.size());
    fbo->getTexture()->bind();
    quadMesh->draw();
}

std::unique_ptr<ImageData> PostProcessing::toImage() {
    return fbo->getTexture()->readData();
}

Framebuffer* PostProcessing::getFramebuffer() const {
    return fbo.get();
}
