#ifndef DX_SHADER_HPP
#define DX_SHADER_HPP

#include <string_view>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

class Shader {
public:
	Shader(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout);
	~Shader();

	void use() const;
private:
	ID3D11VertexShader* m_p_vertexShader;
	ID3D11PixelShader* m_p_pixelShader;
	ID3D11InputLayout* m_p_inputLayout;
public:
	static Shader* loadShader(const std::wstring_view& shaderFile);
};

#endif // !DX_SHADER_HPP