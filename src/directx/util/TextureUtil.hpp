#ifndef TEXTURE_UTIL_HPP
#define TEXTURE_UTIL_HPP

#include <winerror.h>

struct ID3D11Texture2D;

class TextureUtil {
public:
	static HRESULT stageTexture(ID3D11Texture2D* src, ID3D11Texture2D** dst);
	static HRESULT readPixels(ID3D11Texture2D* src, void* dst);
};

#endif // TEXTURE_UTIL_HPP
