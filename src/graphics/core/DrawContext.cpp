#include "DrawContext.hpp"

#include <utility>

#ifdef USE_DIRECTX
#include "../../directx/window/DXDevice.hpp"
#include "../../directx/graphics/DXFramebuffer.hpp"
#elif USE_OPENGL
#include <GL/glew.h>
#include "Framebuffer.hpp"
#endif // USE_DIRECTX

#include "Batch2D.hpp"
#include "../../window/Window.hpp"

static void set_blend_mode(BlendMode mode) {
    switch (mode) {
#ifdef USE_DIRECTX
    case BlendMode::normal:
        DXDevice::setBlendFunc(D3D11_BLEND::D3D11_BLEND_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
            D3D11_BLEND::D3D11_BLEND_ONE, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD);
        break;
    case BlendMode::addition:
        DXDevice::setBlendFunc(D3D11_BLEND::D3D11_BLEND_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_ONE, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
            D3D11_BLEND::D3D11_BLEND_ONE, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD);
        break;
    case BlendMode::inversion:
        DXDevice::setBlendFunc(D3D11_BLEND::D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
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
    Viewport  viewport,
    Batch2D* g2d
) : parent(parent), 
    viewport(std::move(viewport)),
    g2d(g2d) 
{}

DrawContext::~DrawContext() {
    if (g2d) {
        g2d->flush();
    }

    while (scissorsCount--) {
        Window::popScissor();
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

    Window::viewport(
        0, 0,
        parent->viewport.getWidth(), 
        parent->viewport.getHeight()
    );

    if (depthMask != parent->depthMask) {
#ifdef USE_DIRECTX
        DXDevice::setWriteDepthEnabled(parent->depthMask);
#elif USE_OPENGL
        glDepthMask(parent->depthMask);
#endif // USE_DIRECTX
    }
    if (depthTest != parent->depthTest) {
#ifdef USE_DIRECTX
        DXDevice::setDepthTest(!depthTest);
#elif USE_OPENGL
        if (depthTest) glDisable(GL_DEPTH_TEST);
        else glEnable(GL_DEPTH_TEST);
#endif // USE_DIRECTX
    }
    if (cullFace != parent->cullFace) {
#ifdef USE_DIRECTX
        DXDevice::setCullFace(!cullFace);
#elif USE_OPENGL
        if (cullFace) glDisable(GL_CULL_FACE);
        else glEnable(GL_CULL_FACE);
#endif // USE_DIRECTX
    }
    if (blendMode != parent->blendMode) {
        set_blend_mode(parent->blendMode);
    }
}

const Viewport& DrawContext::getViewport() const {
    return viewport;
}

Batch2D* DrawContext::getBatch2D() const {
    return g2d;
}

DrawContext DrawContext::sub() const {
    auto ctx = DrawContext(this, viewport, g2d);
    ctx.depthTest = depthTest;
    ctx.cullFace = cullFace;
    return ctx;
}

void DrawContext::setViewport(const Viewport& viewport) {
    this->viewport = viewport;
    Window::viewport(
        0, 0,
        viewport.getWidth(),
        viewport.getHeight()
    );
}

void DrawContext::setFramebuffer(Framebuffer* fbo) {
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
    DXDevice::setWriteDepthEnabled(flag);
#elif USE_OPENGL
    glDepthMask(GL_FALSE + flag);
#endif // USE_DIRECTX
}

void DrawContext::setDepthTest(bool flag) {
    if (depthTest == flag)
        return;
    depthTest = flag;
#ifdef USE_DIRECTX
	DXDevice::setDepthTest(depthTest);
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
	DXDevice::setCullFace(cullFace);
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

void DrawContext::setScissors(glm::vec4 area) {
    Window::pushScissor(area);
    scissorsCount++;
}
