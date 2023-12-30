#ifndef DX_SKYBOX_HPP
#define DX_SKYBOX_HPP

#include "../../typedefs.h"

class Mesh;
class Shader;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

class Skybox {
public:
    Skybox(uint size, Shader* shader);
    ~Skybox();

    void draw(Shader* shader);

    void refresh(float t, float mie, uint quality);
    void bind() const;
    void unbind() const;
    bool isReady() const {
        return ready;
    }
private:
    uint m_size;
    bool ready = false;

    ID3D11Texture2D* m_p_texture;
    ID3D11RenderTargetView* m_p_renderTargets[6];
    ID3D11ShaderResourceView* m_p_resourceView;
    Shader* m_p_shader;
    Mesh* m_p_mesh;
};

#endif // !DX_SKYBOX_HPP
