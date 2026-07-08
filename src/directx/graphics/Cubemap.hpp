#pragma once

#include "Texture.hpp"

class Cubemap : public Texture {
public:
    Cubemap(uint width, uint height, ImageFormat format);
    Cubemap(ID3D11Texture2D* texture);
};