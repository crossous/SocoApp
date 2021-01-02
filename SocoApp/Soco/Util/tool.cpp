#include "tool.h"

std::ostream& XM_CALLCONV operator<<(std::ostream& os, DirectX::FXMVECTOR v)
{
	DirectX::XMFLOAT4 dest;
	DirectX::XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ", " << dest.w << ")";
	return os;
}
std::ostream& XM_CALLCONV operator<< (std::ostream& os, DirectX::FXMMATRIX m)
{
	//for (int i = 0; i < 4; i++)
	//{
	//	os << DirectX::XMVectorGetX(m.r[i]) << "\t";
	//	os << DirectX::XMVectorGetY(m.r[i]) << "\t";
	//	os << DirectX::XMVectorGetZ(m.r[i]) << "\t";
	//	os << DirectX::XMVectorGetW(m.r[i]) << "\n";
	//}

	os << m.r[0] << "\n";
	os << m.r[1] << "\n";
	os << m.r[2] << "\n";
	os << m.r[3] << "\n";
	return os;
}

std::ostream& XM_CALLCONV operator<< (std::ostream& os, DirectX::XMFLOAT4X4 m) {
	std::cout << m._11 << ", " << m._12 << ", " << m._13 << ", " << m._14 << "\n";
	std::cout << m._21 << ", " << m._22 << ", " << m._23 << ", " << m._24 << "\n";
	std::cout << m._31 << ", " << m._32 << ", " << m._33 << ", " << m._34 << "\n";
	std::cout << m._41 << ", " << m._42 << ", " << m._43 << ", " << m._44 << "\n";
	return os;
}

std::ostream& XM_CALLCONV operator<< (std::ostream& os, DirectX::XMFLOAT4 v) {
	std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
	return os;
}

std::ostream& XM_CALLCONV operator<< (std::ostream& os, DirectX::XMFLOAT3 v) {
	std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}