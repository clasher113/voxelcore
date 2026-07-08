#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <stack>
#include <unordered_map>
#include <string>
#include <d3dcommon.h>

class ResPaths;

class ShaderInclude : public ID3DInclude {
public:
    HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
    HRESULT __stdcall Close(LPCVOID pData);

    static void setDefined(const std::string& macro, bool defined);
    static void setPaths(const ResPaths* paths);
private:
    friend class Shader;

    ShaderInclude();

    std::stack<std::string> m_directoryStack;
    std::stack<D3D_INCLUDE_TYPE> m_includeTypeStack;
    static inline std::unordered_map<std::string, std::string> s_m_macros;
};