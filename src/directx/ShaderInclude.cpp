#include "ShaderInclude.hpp"
#include "../files/files.hpp"

#include <filesystem>
#include <cassert>

namespace fs = std::filesystem;

ShaderInclude::ShaderInclude(const fs::path& rootDir) : m_rootDir(rootDir) {

}

HRESULT __stdcall ShaderInclude::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) {
    
    m_includeTypeStack.push(IncludeType);

    try {
        fs::path finalPath;
        switch (IncludeType) {
            case D3D_INCLUDE_LOCAL:  // #include "FILE"
                finalPath = std::filesystem::absolute(m_rootDir);
                if (!m_directoryStack.empty() && !m_directoryStack.top().empty()) {
                    finalPath = finalPath / (m_directoryStack.top());
                }
                finalPath.append(pFileName);

                m_directoryStack.push(fs::relative(finalPath, m_rootDir).parent_path());
                break;
            case D3D_INCLUDE_SYSTEM:  // #include <FILE>
                finalPath = std::filesystem::absolute(m_rootDir) / pFileName;
                break;
            default:
                assert(0);
        }

        std::string file = files::read_string(finalPath);

        uint32_t fileSize = file.size();

        if (fileSize) {
            *pBytes = fileSize;
            uint8_t* data = reinterpret_cast<uint8_t*>(std::malloc(*pBytes));
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
