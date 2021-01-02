#pragma once
#include <iostream>
#include <DirectXMath.h>
#include <vector>

std::ostream& XM_CALLCONV operator<< (std::ostream& os, DirectX::FXMVECTOR v);
std::ostream& XM_CALLCONV operator<< (std::ostream& os, DirectX::FXMMATRIX m);
std::ostream& XM_CALLCONV operator<< (std::ostream& os, DirectX::XMFLOAT4X4 m);
std::ostream& XM_CALLCONV operator<< (std::ostream& os, DirectX::XMFLOAT4 v);
std::ostream& XM_CALLCONV operator<< (std::ostream& os, DirectX::XMFLOAT3 v);

template<class T>
inline std::ostream& operator<< (std::ostream& os, std::vector<T>& v) {
	std::cout << "vector(";
	for (T& ele : v)
	{
		std::cout << ele << ", ";
	}
	std::cout << ")";
	return os;
}

