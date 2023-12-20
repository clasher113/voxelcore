#include "DXMathHelper.hpp"
#include "glm/gtc/type_ptr.hpp"

DirectX::XMFLOAT4X4 toFloat4x4(const DirectX::XMMATRIX& matrix) {
	DirectX::XMFLOAT4X4 result{};
	DirectX::XMStoreFloat4x4(&result, matrix);
	return result;
}

DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& left, const DirectX::XMFLOAT3& right) {
	DirectX::XMFLOAT3 result{};
	result.x = left.x + right.x;
	result.y = left.y + right.y;
	result.z = left.z + right.z;
	return result;
}

inline DirectX::XMFLOAT4X4 operator*(const DirectX::XMFLOAT4X4& left, const DirectX::XMFLOAT4X4& right) {
	return toFloat4x4(DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&right), DirectX::XMLoadFloat4x4(&left)));
}

inline DirectX::XMFLOAT3 glm2dxm(const glm::vec3& vector3) {
	return DirectX::XMFLOAT3(vector3.x, vector3.y, vector3.z);
}

inline DirectX::XMFLOAT4X4 glm2dxm(const glm::mat4& matrix) {
	return DirectX::XMFLOAT4X4(glm::value_ptr(matrix));
}

inline glm::mat4 dxm2glm(DirectX::XMFLOAT4X4 matrix) {
	return glm::make_mat4x4(&matrix._11);
}

inline DirectX::XMFLOAT4X4 transpose(const DirectX::XMFLOAT4X4& matrix) {
	return toFloat4x4(DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&matrix)));
}
