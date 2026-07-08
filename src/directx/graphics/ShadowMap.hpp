#pragma once

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11DepthStencilView;

class ShadowMap {
public:
    ShadowMap(int resolution);
    ~ShadowMap();

    void bind();
    void unbind();
    ID3D11ShaderResourceView* getSRV() const;
    int getResolution() const;
private:
    ID3D11Texture2D* m_p_depthMap;
    ID3D11ShaderResourceView* m_p_resourceView;
    ID3D11DepthStencilView* m_p_depthStencil;

    int resolution;
};