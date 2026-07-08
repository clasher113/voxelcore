#include "LineBatch.hpp"

#ifdef USE_DIRECTX
#include "directx/graphics/LineRenderer.hpp"
#include "directx/graphics/Mesh.hpp"
#elif USE_OPENGL
#include "Mesh.hpp"
#include <GL/glew.h>
#endif // USE_DIRECTX


LineBatch::LineBatch(size_t capacity) : capacity(capacity) {

    buffer = std::make_unique<LineVertex[]>(capacity * 2);
    mesh = std::make_unique<Mesh<LineVertex>>(buffer.get(), 0);
    index = 0;
}

LineBatch::~LineBatch(){
}

void LineBatch::line(
    float x1, float y1,
    float z1, float x2,
    float y2, float z2,
    float r, float g, float b, float a
) {
    if (index + 2 >= capacity) {
        flush();
    }
    buffer[index].position = {x1,y1,z1};
    buffer[index].color = {r,g,b,a};
    index++;

    buffer[index].position = {x2,y2,z2};
    buffer[index].color = {r,g,b,a};
    index++;
}

void LineBatch::box(float x, float y, float z, float w, float h, float d,
        float r, float g, float b, float a) {
    w *= 0.5f;
    h *= 0.5f;
    d *= 0.5f;

    line(x-w, y-h, z-d, x+w, y-h, z-d, r,g,b,a);
    line(x-w, y+h, z-d, x+w, y+h, z-d, r,g,b,a);
    line(x-w, y-h, z+d, x+w, y-h, z+d, r,g,b,a);
    line(x-w, y+h, z+d, x+w, y+h, z+d, r,g,b,a);

    line(x-w, y-h, z-d, x-w, y+h, z-d, r,g,b,a);
    line(x+w, y-h, z-d, x+w, y+h, z-d, r,g,b,a);
    line(x-w, y-h, z+d, x-w, y+h, z+d, r,g,b,a);
    line(x+w, y-h, z+d, x+w, y+h, z+d, r,g,b,a);

    line(x-w, y-h, z-d, x-w, y-h, z+d, r,g,b,a);
    line(x+w, y-h, z-d, x+w, y-h, z+d, r,g,b,a);
    line(x-w, y+h, z-d, x-w, y+h, z+d, r,g,b,a);
    line(x+w, y+h, z-d, x+w, y+h, z+d, r,g,b,a);
}

void LineBatch::flush(){
    if (index == 0)
        return;
    mesh->reload(buffer.get(), index);
#ifdef USE_DIRECTX
    LineRenderer::draw(*mesh);
#elif USE_OPENGL
    mesh->draw(GL_LINES);
#endif // USE_DIRECTX
    index = 0;
}

void LineBatch::lineWidth(float width) {
#ifdef USE_DIRECTX
    LineRenderer::setWidth(width);
#elif USE_OPENGL
    glLineWidth(width);
#endif // USE_DIRECTX
}
