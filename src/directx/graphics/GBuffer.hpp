#pragma once

#include "typedefs.hpp"
#include "graphics/core/commons.hpp"

class ImageData;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;

class GBuffer : public Bindable {
public:
    GBuffer(uint width, uint height);
    ~GBuffer() override;

    void bind() override;
    void bindSSAO() const;
    void unbind() override;

    void bindBuffers() const;
    void bindSSAOBuffer() const;

    void bindDepthBuffer(ID3D11Texture2D* drawFbo);

    void resize(uint width, uint height);

    uint getWidth() const;
    uint getHeight() const;

    std::unique_ptr<ImageData> toImage() const;
private:
    uint m_width;
    uint m_height;

    ID3D11Texture2D* m_p_colorBuffer = nullptr;
    ID3D11RenderTargetView* m_p_colorBufferView = nullptr;
    ID3D11ShaderResourceView* m_p_colorResourceView = nullptr;

    ID3D11Texture2D* m_p_positionsBuffer = nullptr;
    ID3D11RenderTargetView* m_p_positionsBufferView = nullptr;
    ID3D11ShaderResourceView* m_p_positionsResourceView = nullptr;

    ID3D11Texture2D* m_p_normalsBuffer = nullptr;
    ID3D11RenderTargetView* m_p_normalsBufferView = nullptr;
    ID3D11ShaderResourceView* m_p_normalsResourceView = nullptr;

    ID3D11Texture2D* m_p_emissionBuffer = nullptr;
    ID3D11RenderTargetView* m_p_emissionBufferView = nullptr;
    ID3D11ShaderResourceView* m_p_emissionResourceView = nullptr;

    ID3D11Texture2D* m_p_depthBuffer = nullptr;
    ID3D11DepthStencilView* m_p_depthStencil = nullptr;

    ID3D11Texture2D* m_p_ssaoBuffer = nullptr;
    ID3D11RenderTargetView* m_p_ssaoBufferView = nullptr;
    ID3D11ShaderResourceView* m_p_ssaoResourceView = nullptr;

    void createColorBuffer();
    void createPositionsBuffer();
    void createNormalsBuffer();
    void createEmissionBuffer();
    void createDepthBuffer();
    void createSSAOBuffer();

    void releaseResources();

    void unbindBuffers() const;
};