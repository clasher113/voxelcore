#ifndef DX_SHADER_HPP
#define DX_SHADER_HPP

#include "../ConstantBuffer.hpp"

#include <string_view>
#include <memory>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

class Shader : public ConstantBuffer {
public:
	Shader(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout, const ConstantBufferData& cbuffData);
	~Shader();

	void use();
private:
	ID3D11VertexShader* m_p_vertexShader;
	ID3D11PixelShader* m_p_pixelShader;
	ID3D11InputLayout* m_p_inputLayout;
public:
	static void compileShader(const std::wstring_view& shaderFile, ID3D10Blob** shader, ShaderType shaderType);
	static std::unique_ptr<Shader> loadShader(const std::wstring_view& shaderFile);
};

#endif // !DX_SHADER_HPP