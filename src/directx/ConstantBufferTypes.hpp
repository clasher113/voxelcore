#ifdef USE_DIRECTX
#ifndef CONTENT_BUFFER_TYPES_HPP
#define CONTENT_BUFFER_TYPES_HPP

#include <DirectXMath.h>

struct CB_Main {
	DirectX::XMFLOAT4X4 model;
	DirectX::XMFLOAT4X4 projection;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT3 skyLightColor;
	float gamma;
	DirectX::XMFLOAT3 cameraPos;
	float torchLightDistance;
	DirectX::XMFLOAT3 torchLightColor;
	float fogFactor;
	DirectX::XMFLOAT3 fogColor;
	float fogCurve;
	int cubemap;
};

struct CB_Lines {
	DirectX::XMFLOAT4X4 projView;
};

struct CB_UI {
	DirectX::XMFLOAT4X4 projView;
};

struct CB_UI3D {
	DirectX::XMFLOAT4X4 projView;
	DirectX::XMFLOAT4X4 apply;
};

struct CB_Background {
	DirectX::XMFLOAT4X4 view;
	float ar;
	float zoom;
};

struct CB_Skybox {
	DirectX::XMFLOAT3 xAxis;
	int quality;
	DirectX::XMFLOAT3 yAxis;
	float mie;
	DirectX::XMFLOAT3 uAxis;
	float fog;
	DirectX::XMFLOAT3 lightDir;
};

#endif // !CONTENT_BUFFER_TYPES_HPP
#endif // USE_DIRECTX