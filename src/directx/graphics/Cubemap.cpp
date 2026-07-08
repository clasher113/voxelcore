#ifdef USE_DIRECTX
#include "Cubemap.hpp"

#include "directx/window/Device.hpp"
#include "directx/util/Error.hpp"

Cubemap::Cubemap(uint width, uint height, ImageFormat format) : Texture(nullptr) {
	ID3D11Device* device = Device::getDevice();

	m_description = {
		/* UINT Width */					width,
		/* UINT Height */					height,
		/* UINT MipLevels */				0U,
		/* UINT ArraySize */				6U,
		/* DXGI_FORMAT Format */			DXGI_FORMAT_R8G8B8A8_UNORM,
		/* DXGI_SAMPLE_DESC SampleDesc */{
			/* UINT Count */	1,
			/* UINT Quality */	0
		},
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				D3D11_BIND_SHADER_RESOURCE,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				D3D11_RESOURCE_MISC_TEXTURECUBE
	};

	CHECK_ERROR2(device->CreateTexture2D(&m_description, nullptr, &m_p_texture),
		L"Failed to create texture");

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{
		/* DXGI_FORMAT Format */				m_description.Format,
		/* D3D11_SRV_DIMENSION ViewDimension */	D3D11_SRV_DIMENSION_TEXTURECUBE,
		/* D3D11_TEX2D_SRV Texture2D */{
			/* UINT MostDetailedMip */	0U,
			/* UINT MipLevels */		1U
		}
	};

	CHECK_ERROR2(device->CreateShaderResourceView(m_p_texture, &srvDesc, &m_p_resourceView),
		L"Failed to create shader resource view");
}

Cubemap::Cubemap(ID3D11Texture2D* texture) : Texture(texture) {
}

#endif // USE_DIRECTX