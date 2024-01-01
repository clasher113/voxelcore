#ifndef DX_SHADER_HPP
#define DX_SHADER_HPP

#include <string_view>
#include <unordered_map>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

struct CBuffVar {
	unsigned int startOffset;
	unsigned int size;
};

struct CBuff {
	unsigned char* data;
	std::unordered_map<const char*, CBuffVar> vars;
};

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