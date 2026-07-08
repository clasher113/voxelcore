#pragma once

struct ID3D11Device;
struct ID3D10Blob;
struct ID3D11InputLayout;

class InputLayoutBuilder {
public:
	static bool create(ID3D11Device* device, ID3D10Blob* shader, ID3D11InputLayout** inputLayout);
};
