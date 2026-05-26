#include "assets_util.hpp"

#include "assets/Assets.hpp"
#include "graphics/core/Atlas.hpp"

#ifdef USE_DIRECTX
#include "directx/graphics/DXTexture.hpp"
#elif USE_OPENGL
#include "graphics/core/Texture.hpp"
#endif // USE_DIRECTX

util::TextureRegion util::get_texture_region(
    const Assets& assets, const std::string& name, const std::string& fallback
) {
    size_t sep = name.find(':');
    if (sep == std::string::npos) {
        return {assets.get<Texture>(name), UVRegion(0,0,1,1)};
    } else {
        auto atlas = assets.get<Atlas>(name.substr(0, sep));
        if (atlas) {
            if (sep == name.length() - 1) {
                return {atlas->getTexture(), UVRegion(0,0,1,1)};
            }
            if (auto reg = atlas->getIf(name.substr(sep+1))) {
                return {atlas->getTexture(), *reg};
            } else if (!fallback.empty()){
                return util::get_texture_region(assets, fallback, "");
            }
        }
    }
    return {nullptr, UVRegion()};
}
