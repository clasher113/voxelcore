#ifndef SHADER_INCLUDE_HPP
#define SHADER_INCLUDE_HPP

#include <filesystem>
#include <stack>
#include <d3dcommon.h>

class ShaderInclude : public ID3DInclude {
public:
    ShaderInclude(const std::filesystem::path& shaderDir);

    HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
    HRESULT __stdcall Close(LPCVOID pData);
private:
    std::filesystem::path m_rootDir;
    std::stack<std::filesystem::path> m_directoryStack;
    std::stack<D3D_INCLUDE_TYPE> m_includeTypeStack;
};

#endif  // !SHADER_INCLUDE_HPP
