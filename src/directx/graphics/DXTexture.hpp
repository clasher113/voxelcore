#ifndef DX_TEXTURE_HPP
#define DX_TEXTURE_HPP

#include <d3d11_1.h>

#include "../../typedefs.h"

typedef unsigned int UINT;

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
class ImageData;

class Texture {
public:
	Texture(ID3D11Texture2D* texture, const D3D11_TEXTURE2D_DESC& description);
	Texture(ubyte* data, UINT width, UINT height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
	~Texture();

	int getWidth() const { return m_description.Width; };
	int getHeight() const { return m_description.Height; };

	void bind() const;
	void reload(ubyte* data);

	static Texture* from(const ImageData* image);
private:
	D3D11_TEXTURE2D_DESC m_description;
	ID3D11Texture2D* m_p_texture;
	ID3D11ShaderResourceView* m_p_resourceView;
};

#endif // !DX_TEXTURE_HPP
