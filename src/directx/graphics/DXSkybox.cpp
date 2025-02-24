#ifdef USE_DIRECTX
#include "DXSkybox.hpp"
#include "window/Camera.hpp"
#include "DXMesh.hpp"
#include "directx/window/DXDevice.hpp"
#include "directx/util/DXError.hpp"
#include "directx/ConstantBuffer.hpp"
#include "DXShader.hpp"
#include "DXTexture.hpp"
#include "directx/math/DXMathHelper.hpp"
#include "window/Window.hpp"
#include "graphics/core/DrawContext.hpp"
#include "assets/Assets.hpp"
#include "graphics/core/Batch3D.hpp"
#include "maths/UVRegion.hpp"

#include <glm\glm.hpp>
#include <d3d11_1.h>

#ifndef M_PI
#define M_PI 3.141592
#endif // M_PI
const int STARS_COUNT = 3000;
const int STARS_SEED = 632;

const DirectX::XMFLOAT3 xaxs[] = {
	{ 0.f, 0.f,-1.f },
	{ 0.f, 0.f, 1.f },
	{-1.f, 0.f, 0.f },

	{-1.f, 0.f, 0.f },
	{-1.f, 0.f, 0.f },
	{ 1.f, 0.f, 0.f },
};
const DirectX::XMFLOAT3 yaxs[] = {
	{ 0.f,-1.f, 0.f },
	{ 0.f,-1.f, 0.f },
	{ 0.f, 0.f, 1.f },

	{ 0.f, 0.f,-1.f },
	{ 0.f,-1.f, 0.f },
	{ 0.f,-1.f, 0.f },
};
const DirectX::XMFLOAT3 zaxs[] = {
	{ 1.f, 0.f, 0.f },
	{-1.f, 0.f, 0.f },
	{ 0.f,-1.f, 0.f },

	{ 0.f, 1.f, 0.f },
	{ 0.f, 0.f,-1.f },
	{ 0.f, 0.f, 1.f },
};

Skybox::Skybox(uint size, Shader& shader) :
	m_size(size),
	m_p_shader(shader),
	m_p_batch3d(new Batch3D(4096)),
	random()
{
	auto device = DXDevice::getDevice();

	DXGI_SAMPLE_DESC sampleDesc {
		/* UINT Count */	1U,
		/* UINT Quality */	0U
	};

	D3D11_TEXTURE2D_DESC description {
		/* UINT Width */					m_size,
		/* UINT Height */					m_size,
		/* UINT MipLevels */				1U,
		/* UINT ArraySize */				6U,
		/* DXGI_FORMAT Format */			DXGI_FORMAT_R8G8B8A8_UNORM,
		/* DXGI_SAMPLE_DESC SampleDesc */	sampleDesc,
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				D3D11_RESOURCE_MISC_TEXTURECUBE
	};

	CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &m_p_texture),
		L"Failed to create texture");

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc {
		/* DXGI_FORMAT Format */				description.Format,
		/* D3D11_SRV_DIMENSION ViewDimension */	D3D11_SRV_DIMENSION_TEXTURECUBE,
		/* D3D11_TEX2D_SRV Texture2D */			{
			/* UINT MostDetailedMip */	0U,
			/* UINT MipLevels */		1U
		}
	};

	CHECK_ERROR2(device->CreateShaderResourceView(m_p_texture, &srvDesc, &m_p_resourceView),
		L"Failed to create shader resource view");

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc {
		/* DXGI_FORMAT Format */					description.Format,
		/* D3D11_RTV_DIMENSION ViewDimension */		D3D11_RTV_DIMENSION_TEXTURE2DARRAY,
		/* D3D11_TEX2D_ARRAY_RTV Texture2DArray */	{
			/* UINT MipSlice */			0U,
			/* UINT FirstArraySlice */	1U
		}
	};
	rtvDesc.Texture2DArray.ArraySize = 1;

	for (size_t i = 0; i < 6; i++) {
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		CHECK_ERROR1(device->CreateRenderTargetView(m_p_texture, &rtvDesc, &m_p_renderTargets[i]));
	}

	float vertices[]{
		-1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f, -1.0f,  1.0f, 1.0f,
	};
	VertexAttribute attrs[]{ {2}, {0} };
	m_p_mesh = new Mesh(vertices, 6, attrs);

	sprites.push_back(skysprite {
        "misc/moon",
        M_PI*0.5f,
        4.0f,
        false
    });

    sprites.push_back(skysprite {
        "misc/sun",
        M_PI*1.5f,
        4.0f,
        true
    });
}

Skybox::~Skybox() {
	m_p_texture->Release();
	for (size_t i = 0; i < 6; i++) {
		m_p_renderTargets[i]->Release();
	}
	m_p_resourceView->Release();
	delete m_p_mesh;
	delete m_p_batch3d;
}

void Skybox::drawBackground(const Camera& camera, const Assets& assets, int width, int height) {
	Shader* backShader = assets.get<Shader>("background");
	backShader->uniformMatrix("u_view", camera.getView(false));
	backShader->uniform1f("u_zoom", camera.zoom * camera.getFov() / (M_PI * 0.5f));
	backShader->uniform1f("u_ar", float(width) / float(height));
	backShader->use();

	DXDevice::getContext()->PSSetShaderResources(0, 1, &m_p_resourceView);
	DXDevice::setDepthTest(false);
	m_p_mesh->draw();
	//DXDevice::setDepthTest(true);
}

void Skybox::drawStars(float angle, float opacity) {
	m_p_batch3d->texture(nullptr);
	random.setSeed(STARS_SEED);
	for (int i = 0; i < STARS_COUNT; i++) {
		float rx = (random.randFloat()) - 0.5f;
		float ry = (random.randFloat()) - 0.5f;
		float z = (random.randFloat()) - 0.5f;
		float x = rx * sin(angle) + ry * -cos(angle);
		float y = rx * cos(angle) + ry * sin(angle);

		float sopacity = random.randFloat();
		if (y < 0.0f)
			continue;

		sopacity *= (0.2f + sqrt(cos(angle)) * 0.5) - 0.05;
		glm::vec4 tint(1, 1, 1, sopacity * opacity);
		m_p_batch3d->point(glm::vec3(x, y, z), tint);
	}
	m_p_batch3d->flushPoints();
}

void Skybox::draw(const DrawContext& pctx, const Camera& camera, const Assets& assets, float daytime, float fog) {
	const Viewport& viewport = pctx.getViewport();
	int width = viewport.getWidth();
	int height = viewport.getHeight();

	drawBackground(camera, assets, width, height);

	DrawContext ctx = pctx.sub();
	ctx.setBlendMode(BlendMode::addition);

	Shader* shader = assets.get<Shader>("ui3d");
	shader->uniformMatrix("u_projview", camera.getProjView(false));
	shader->uniformMatrix("u_apply", glm::mat4(1.0f));
	shader->use();
	m_p_batch3d->begin();

	float angle = daytime * M_PI * 2;
	float opacity = glm::pow(1.0f - fog, 7.0f);

	for (auto& sprite : sprites) {
		m_p_batch3d->texture(assets.get<Texture>(sprite.texture));

		float sangle = daytime * M_PI * 2 + sprite.phase;
		float distance = sprite.distance;

		glm::vec3 pos(-cos(sangle) * distance, sin(sangle) * distance, 0);
		glm::vec3 up(-sin(-sangle), cos(-sangle), 0.0f);
		glm::vec4 tint(1, 1, 1, opacity);
		if (!sprite.emissive) {
			tint *= 0.6f + cos(angle) * 0.4;
		}
		m_p_batch3d->sprite(pos, glm::vec3(0, 0, 1),
			up, 1, 1, UVRegion(), tint);
	}

	drawStars(angle, opacity);
}

void Skybox::refresh(const DrawContext& pctx, float t, float mie, uint quality) {
	frameid++;
	float dayTime = t;

	DrawContext ctx = pctx.sub();
	ctx.setDepthMask(false);
	ctx.setDepthTest(false);
	ctx.setViewport(Viewport(m_size, m_size));

	ready = true;
	m_p_shader.use();

	t *= M_PI * 2.0f;

	lightDir = glm::normalize(glm::vec3(sin(t), -cos(t), 0.0f));
	m_p_shader.uniform1i("c_quality", quality);
	m_p_shader.uniform1f("c_mie", mie);
	m_p_shader.uniform1f("c_fog", mie - 1.0f);
	m_p_shader.uniform3f("c_lightDir", lightDir);
	m_p_shader.uniform1f("c_dayTime", dayTime);

	if (glm::abs(mie - prevMie) + glm::abs(t - prevT) >= 0.01) {
		for (uint face = 0; face < 6; face++) {
			refreshFace(face);
		}
	} else {
		uint face = frameid % 6;
		refreshFace(face);
	}
	prevMie = mie;
	prevT = t;

	DXDevice::resetRenderTarget();
}

void Skybox::refreshFace(uint face) {
	DXDevice::getContext()->OMSetRenderTargets(1, &m_p_renderTargets[face], nullptr);

	m_p_shader.uniform3f("c_xAxis", xaxs[face]);
	m_p_shader.uniform3f("c_yAxis", yaxs[face]);
	m_p_shader.uniform3f("c_zAxis", zaxs[face]);

	m_p_shader.applyChanges();
	m_p_mesh->draw();
}

void Skybox::bind() const {
	auto context = DXDevice::getContext();
	context->VSSetShaderResources(1, 1, &m_p_resourceView);
	context->PSSetShaderResources(1, 1, &m_p_resourceView);
}

void Skybox::unbind() const {
	auto context = DXDevice::getContext();
	ID3D11ShaderResourceView* nullSRV = { nullptr };
	context->VSSetShaderResources(1, 1, &nullSRV);
	context->PSSetShaderResources(1, 1, &nullSRV);
}

#endif // USE_DIRECTX