#ifdef USE_DIRECTX
#include "DXLine.hpp"
#include "../window/DXDevice.hpp"
#include "../../util/stringutil.h"
#include "../util/DXError.hpp"
#include "DXMesh.hpp"

#include <d3dcommon.h>
#include <d3dcompiler.h>

float DXLine::m_width = 1.f;
ID3D11GeometryShader* DXLine::s_m_p_geometryShader = nullptr;
ConstantBuffer<CB_LineWidth>* DXLine::s_m_p_cbLineWidth;

void DXLine::init() {
    auto device = DXDevice::getDevice();

    ID3D10Blob* GS;
    ID3D10Blob* errorMsg = nullptr;
    HRESULT errorCode = S_OK;
    UINT flag1 = 0, flag2 = 0;
#ifdef _DEBUG
    flag1 = D3DCOMPILE_DEBUG;
    flag2 = D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
    D3DCompileFromFile(L"res\\dx-shaders\\lines.hlsl", 0, 0, "GShader", "gs_4_0", flag1, flag2, &GS, &errorMsg);
    CHECK_ERROR2(errorCode, L"Failed to compile vertex shader:\n" +
        util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
    if (errorMsg != nullptr) {
        DXError::throwWarn(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
        errorMsg->Release();
        errorMsg = nullptr;
    }

    CHECK_ERROR1(device->CreateGeometryShader(GS->GetBufferPointer(), GS->GetBufferSize(), NULL, &s_m_p_geometryShader));

    GS->Release();

    s_m_p_cbLineWidth = new ConstantBuffer<CB_LineWidth>;

    s_m_p_cbLineWidth->data.lineWidth = m_width;
    s_m_p_cbLineWidth->applyChanges();
}

void DXLine::setWidth(float width) {
    if (m_width == width) return;
    m_width = width;

    s_m_p_cbLineWidth->data.lineWidth = m_width;
    s_m_p_cbLineWidth->applyChanges();
}

void DXLine::draw(Mesh* mesh) {
    auto context = DXDevice::getContext();
    s_m_p_cbLineWidth->bind(ShaderType::GEOMETRY, 1u);
    context->GSSetShader(s_m_p_geometryShader, 0, 0);
    mesh->draw(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    context->GSSetShader(nullptr, 0, 0);
}

#endif // USE_DIRECTX