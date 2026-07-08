#pragma once

#include "directx/ConstantBuffer.hpp"

#include <string>
#include <memory>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D10Blob;

struct ShaderBundle {
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* inputLayout;
};

class Shader : public ConstantBuffer {
public:
	Shader(const ShaderBundle& shaderBundle, const ConstantBufferData& cbuffData, std::string sourceCode);
	~Shader();

	void use();
	void recompile();
private:
	std::string m_sourceCode;

	ID3D11VertexShader* m_p_vertexShader;
	ID3D11PixelShader* m_p_pixelShader;
	ID3D11InputLayout* m_p_inputLayout;

	static Shader* used;
public:
	static ID3D10Blob* compileShader(const std::string& shaderSource, ShaderType shaderType);
	static std::unique_ptr<Shader> loadShader(const std::string& fileName, const std::string& shaderSource);
	static Shader& getUsed();
};