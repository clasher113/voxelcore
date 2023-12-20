#ifndef DX_SKYBOX_HPP
#define DX_SKYBOX_HPP

#include "../../typedefs.h"

class Mesh;
class Shader;

class Skybox {
    uint fbo;
    uint cubemap;
    uint size;
    Mesh* mesh;
    Shader* shader;
    bool ready = false;
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
};

#endif // !DX_SKYBOX_HPP
