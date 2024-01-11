#ifndef DX_MATH_HELPER_HPP
#define DX_MATH_HELPER_HPP

#include <DirectXMath.h>
#include <glm/glm.hpp>

extern inline DirectX::XMFLOAT4X4 toFloat4x4(const DirectX::XMMATRIX& matrix);
extern inline DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& left, const DirectX::XMFLOAT3& right);
extern inline DirectX::XMFLOAT4X4 operator*(const DirectX::XMFLOAT4X4& left, const DirectX::XMFLOAT4X4& right);
extern inline DirectX::XMFLOAT3 glm2dxm(const glm::vec3& vector3);
extern inline DirectX::XMFLOAT2 glm2dxm(const glm::vec2& vector2);
extern inline DirectX::XMFLOAT4X4 glm2dxm(const glm::mat4& matrix);
extern inline glm::mat4 dxm2glm(DirectX::XMFLOAT4X4 matrix);
extern inline DirectX::XMFLOAT4X4 transpose(const DirectX::XMFLOAT4X4& matrix);

#endif // !DX_MATH_HELPER_HPP