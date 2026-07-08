#ifdef USE_DIRECTX
#include "ShaderInclude.hpp"

#include "io/io.hpp"
#include "io/engine_paths.hpp"
#include "constants.hpp"

#include <cassert>

namespace fs = std::filesystem;

static ResPaths const* p_paths = nullptr;

ShaderInclude::ShaderInclude() {

}

HRESULT __stdcall ShaderInclude::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) {
    
    m_includeTypeStack.push(IncludeType);

    try {
        const std::string fileName = pFileName;
        std::string finalPath;

        switch (IncludeType) {
            case D3D_INCLUDE_LOCAL:  // #include "FILE"
                finalPath = SHADERS_FOLDER;
                if (!m_directoryStack.empty() && !m_directoryStack.top().empty()) {
                    finalPath += '/' + m_directoryStack.top();
                }
                finalPath.append("/"); finalPath.append(pFileName);

                m_directoryStack.push(fs::path(fileName).parent_path().string());
                break;
            case D3D_INCLUDE_SYSTEM:  // #include <FILE>
                finalPath = SHADERS_FOLDER + '/' + fileName;
                break;
            default:
                assert(0);
        }

        const std::string file = io::read_string(p_paths->find(finalPath));
        const uint32_t fileSize = file.size();

        if (fileSize) {
            *pBytes = fileSize;
            uint8_t* data = reinterpret_cast<uint8_t*>(std::malloc(*pBytes));
            if (data == nullptr) {
                return E_OUTOFMEMORY;
            }
            memcpy(data, file.data(), fileSize);
            *ppData = data;
        } else {
            *ppData = nullptr;
            *pBytes = 0;
        }
        return S_OK;
    } catch (std::runtime_error& err) {
        return E_FAIL;
    }
    return E_NOTIMPL;
}

HRESULT __stdcall ShaderInclude::Close(LPCVOID pData) {
    std::free(const_cast<void*>(pData));
    if (m_includeTypeStack.top() == D3D_INCLUDE_TYPE::D3D_INCLUDE_LOCAL) {
        m_directoryStack.pop();
        m_includeTypeStack.pop();
    }
    return S_OK;
}

void ShaderInclude::setDefined(const std::string& macro, bool defined) {
    if (defined) {
        s_m_macros[macro] = "TRUE";
    }
    else {
        s_m_macros.erase(macro);
    }
}

void ShaderInclude::setPaths(const ResPaths* paths) {
    p_paths = paths;
}

#endif // USE_DIRECTX