#include "DrawContext.hpp"

#include <utility>

#ifdef USE_DIRECTX
#include "directx/window/Device.hpp"
#include "directx/graphics/Framebuffer.hpp"
#include "directx/graphics/LineRenderer.hpp"
#elif USE_OPENGL
#include <GL/glew.h>
#include "Framebuffer.hpp"
#endif // USE_DIRECTX

#include "Batch2D.hpp"
#include "window/Window.hpp"

static void set_blend_mode(BlendMode mode) {
    switch (mode) {
#ifdef USE_DIRECTX
    case BlendMode::normal:
        Device::setBlendFunc(D3D11_BLEND::D3D11_BLEND_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
            D3D11_BLEND::D3D11_BLEND_ONE, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD);
        break;
    case BlendMode::addition:
        Device::setBlendFunc(D3D11_BLEND::D3D11_BLEND_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_ONE, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
            D3D11_BLEND::D3D11_BLEND_ONE, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD);
        break;
    case BlendMode::inversion:
        Device::setBlendFunc(D3D11_BLEND::D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
            D3D11_BLEND::D3D11_BLEND_ONE, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD);
        break;
#elif USE_OPENGL
    case BlendMode::normal:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case BlendMode::addition:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case BlendMode::inversion:
        glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
        break;
#endif // USE_DIRECTX
    }
}


DrawContext::DrawContext(
    const DrawContext* parent, 
    Window& window,
    Batch2D* g2d
) : window(window),
    parent(parent), 
    viewport(window.getSize()),
    g2d(g2d),
    flushable(g2d)
{}

DrawContext::~DrawContext() {
    if (flushable) {
        flushable->flush();
    }

    while (scissorsCount--) {
        window.popScissor();
    }

    if (parent == nullptr)
        return;

    if (fbo != parent->fbo) {
        if (fbo) {
            fbo->unbind();
        }
        if (parent->fbo) {
            parent->fbo->bind();
        }
    }
#ifdef USE_DIRECTX
    Device::resizeViewPort(0, 0, parent->viewport.x, parent->viewport.y);
#elif USE_OPENGL
    glViewport(0, 0, parent->viewport.x, parent->viewport.y);
#endif // USE_DIRECTX

    if (depthMask != parent->depthMask) {
#ifdef USE_DIRECTX
        Device::setWriteDepthEnabled(parent->depthMask);
#elif USE_OPENGL
        glDepthMask(parent->depthMask);
#endif // USE_DIRECTX
    }
    if (depthTest != parent->depthTest) {
#ifdef USE_DIRECTX
        Device::setDepthTest(!depthTest);
#elif USE_OPENGL
        if (depthTest) glDisable(GL_DEPTH_TEST);
        else glEnable(GL_DEPTH_TEST);
#endif // USE_DIRECTX
    }
    if (cullFace != parent->cullFace) {
#ifdef USE_DIRECTX
        Device::setCullFace(!cullFace);
#elif USE_OPENGL
        if (cullFace) glDisable(GL_CULL_FACE);
        else glEnable(GL_CULL_FACE);
#endif // USE_DIRECTX
    }
    if (blendMode != parent->blendMode) {
        set_blend_mode(parent->blendMode);
    }
    if (lineWidth != parent->lineWidth) {
#ifdef USE_DIRECTX
        LineRenderer::setWidth(parent->lineWidth);
#elif USE_OPENGL
        glLineWidth(parent->lineWidth);
#endif  // USE_DIRECTX
    }
}

const glm::uvec2& DrawContext::getViewport() const {
    return viewport;
}

Batch2D* DrawContext::getBatch2D() const {
    return g2d;
}

DrawContext DrawContext::sub(Flushable* flushable) const {
    auto ctx = DrawContext(*this);
    ctx.parent = this;
    ctx.flushable = flushable;
    ctx.scissorsCount = 0;
    if (auto batch2D = dynamic_cast<Batch2D*>(flushable)) {
        ctx.g2d = batch2D;
    }
    return ctx;
}

void DrawContext::setViewport(const glm::uvec2& viewport) {
    this->viewport = viewport;
#ifdef USE_DIRECTX
    Device::resizeViewPort(0, 0, viewport.x, viewport.y);
#elif USE_OPENGL
    glViewport(0, 0, viewport.x, viewport.y);
#endif // USE_DIRECTX
}

void DrawContext::setFramebuffer(Bindable* fbo) {
    if (this->fbo == fbo)
        return;
    this->fbo = fbo;
    if (fbo) {
        fbo->bind();
    }
}

void DrawContext::setDepthMask(bool flag) {
    if (depthMask == flag)
        return;
    depthMask = flag;
#ifdef USE_DIRECTX
    Device::setWriteDepthEnabled(flag);
#elif USE_OPENGL
    glDepthMask(GL_FALSE + flag);
#endif // USE_DIRECTX
}

void DrawContext::setDepthTest(bool flag) {
    if (depthTest == flag)
        return;
    depthTest = flag;
#ifdef USE_DIRECTX
	Device::setDepthTest(depthTest);
#elif USE_OPENGL
	if (depthTest) {
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}
#endif // USE_DIRECTX
}

void DrawContext::setCullFace(bool flag) {
    if (cullFace == flag)
        return;
    cullFace = flag;
#ifdef USE_DIRECTX
	Device::setCullFace(cullFace);
#elif USE_OPENGL
	if (cullFace) {
		glEnable(GL_CULL_FACE);
	}
	else {
		glDisable(GL_CULL_FACE);
	}
#endif // USE_DIRECTX
}

void DrawContext::setBlendMode(BlendMode mode) {
    if (blendMode == mode)
        return;
    blendMode = mode;
    set_blend_mode(mode);
}

void DrawContext::setScissors(const glm::vec4& area) {
    window.pushScissor(area);
    scissorsCount++;
}

void DrawContext::setLineWidth(float width) {
    lineWidth = width;
#ifdef USE_DIRECTX
    LineRenderer::setWidth(width);
#elif USE_OPENGL
    glLineWidth(width);
#endif  // USE_DIRECTX
}
