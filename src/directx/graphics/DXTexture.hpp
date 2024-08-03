#ifndef DX_TEXTURE_HPP
#define DX_TEXTURE_HPP

#define NOMINMAX

#include "../../typedefs.hpp"
#include "../../graphics/core/ImageData.hpp"
#include "../ShaderTypes.hpp"

#include <d3d11_1.h>
#include <memory>

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;

class Texture {
public:
	Texture(ID3D11Texture2D* texture);
	Texture(ubyte* data, uint width, uint height, ImageFormat format);
	~Texture();

	UINT getWidth() const { return m_description.Width; };
	UINT getHeight() const { return m_description.Height; };

	void setNearestFilter();

	void bind(unsigned int shaderType = ShaderType::PIXEL, UINT startSlot = 0u) const;
	void reload(ubyte* data);

	virtual std::unique_ptr<ImageData> readData();
	virtual ID3D11Texture2D* getId() const;
	ID3D11ShaderResourceView* getResourceView() const;

	static std::unique_ptr<Texture> from(const ImageData* image);

	static uint MAX_RESOLUTION;
private:
	D3D11_TEXTURE2D_DESC m_description;
	ID3D11Texture2D* m_p_texture;
	ID3D11ShaderResourceView* m_p_resourceView;
};

#endif // !DX_TEXTURE_HPP
