#pragma once

struct ID3D11Texture2D;

class TextureUtil {
public:
	static bool stageTexture(ID3D11Texture2D* src, ID3D11Texture2D** dst);
	static bool readPixels(ID3D11Texture2D* src, void* dst, bool flipY = true);
};