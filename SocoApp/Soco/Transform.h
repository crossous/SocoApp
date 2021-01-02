#pragma once
//#include "../../Common/MathHelper.h"

#include <Windows.h>
#include <DirectXMath.h>
#include <cstdint>

namespace Soco {
class Transform {
private:
	DirectX::XMFLOAT3 localPosition = { 0, 0, 0 };
	DirectX::XMFLOAT4 localRotationQuat = { 0, 0, 0, 1 };
	DirectX::XMFLOAT3 localScale = { 1, 1, 1 };

public:
	Transform* parent = nullptr;

	Transform() {}
	Transform(DirectX::XMFLOAT3 position, Transform* parent = nullptr) : localPosition(position), parent(parent) {}
	Transform(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, Transform* parent = nullptr)
		: localPosition(position), parent(parent) {
		SetLocalRotation(rotation);
	}
	Transform(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale, Transform* parent = nullptr)
		: localPosition(position), localScale(scale), parent(parent) {
		SetLocalRotation(rotation);
	}


	DirectX::XMFLOAT3 GetLocalPosition() { return localPosition; }
	DirectX::XMFLOAT3 GetLocalScale() { return localScale; }

	DirectX::XMVECTOR GetLocalPositionV() { return DirectX::XMLoadFloat3(&localPosition); }
	DirectX::XMVECTOR GetLocalScaleV() { return DirectX::XMLoadFloat3(&localScale); }

	void SetLocalPosition(DirectX::XMFLOAT3 position) {
		this->localPosition = position;
	}

	void SetLocalRotation(DirectX::XMFLOAT3 rotation) {
		DirectX::XMVECTOR quat = QuaternionRotationYawPitchRoll(rotation.x, rotation.y, rotation.z);
		DirectX::XMStoreFloat4(&(this->localRotationQuat), quat);
	}

	void SetLocalScale(DirectX::XMFLOAT3 scale) {
		this->localScale = scale;
	}


	void SetGlobalPosition(DirectX::XMFLOAT3 position) {
		if (parent == nullptr)
			this->localPosition = position;
		else
			DirectX::XMStoreFloat3(&(this->localPosition),
				DirectX::XMVector3TransformCoord(
					DirectX::XMLoadFloat3(&position), DirectX::XMMatrixTranspose(parent->GetGlobalMatrixM())));
	}

	void SetGlobalRotation(DirectX::XMFLOAT3 rotation) {
		DirectX::XMVECTOR globalQuat = QuaternionRotationYawPitchRoll(rotation.x, rotation.y, rotation.z);
		if (parent == nullptr) {
			DirectX::XMStoreFloat4(&(this->localRotationQuat), globalQuat);
		}
		else {
			DirectX::XMVECTOR parentQuat = DirectX::XMQuaternionRotationMatrix(GetGlobalMatrixM());
			DirectX::XMVECTOR localQuat =
				DirectX::XMQuaternionMultiply(globalQuat, DirectX::XMQuaternionInverse(parentQuat));
			DirectX::XMStoreFloat4(&(this->localRotationQuat), globalQuat);
		}
	}

	DirectX::XMVECTOR GetGlobalPositionV() {
		if (parent == nullptr) {
			return DirectX::XMLoadFloat3(&localPosition);
		}
		else {
			return DirectX::XMVector2TransformCoord(DirectX::XMLoadFloat3(&localPosition), parent->GetGlobalMatrixM());
		}
	}

	DirectX::XMFLOAT3 GetGlobalPosition() {
		DirectX::XMFLOAT3 res;
		DirectX::XMStoreFloat3(&res, GetGlobalPositionV());
		return res;
	}

	inline DirectX::XMMATRIX GetLocalMatrixM() {
		return DirectX::XMMatrixAffineTransformation(DirectX::XMLoadFloat3(&localScale), DirectX::XMQuaternionIdentity(),
			DirectX::XMLoadFloat4(&localRotationQuat), DirectX::XMLoadFloat3(&localPosition));
	}

	DirectX::XMMATRIX GetGlobalMatrixM() {
		return parent == nullptr ? GetLocalMatrixM() : GetLocalMatrixM() * parent->GetGlobalMatrixM();
	}

	inline DirectX::XMFLOAT4X4 GetLocalMatrix() {
		DirectX::XMFLOAT4X4 res;
		DirectX::XMStoreFloat4x4(&res, GetLocalMatrixM());
		return res;
	}

	DirectX::XMFLOAT4X4 GetGlobalMatrix() {
		DirectX::XMFLOAT4X4 res;
		DirectX::XMStoreFloat4x4(&res, GetGlobalMatrixM());
		return res;
	}

	DirectX::XMFLOAT3 GetLocalRotationEular() {
		DirectX::XMMATRIX rotationMat = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&localRotationQuat));
		DirectX::XMFLOAT4X4 m;
		DirectX::XMStoreFloat4x4(&m, rotationMat);

		float x, y, z;
		y = atan2(-m._13, m._33);
		x = asin(m._23);
		z = atan2(-m._21, m._22);
		return { x, y, z };
	}

	DirectX::XMFLOAT3 GetGlobalRotationEular() {
		DirectX::XMMATRIX rotationMat = GetGlobalMatrixM();
		DirectX::XMFLOAT4X4 m;
		DirectX::XMStoreFloat4x4(&m, rotationMat);

		float x, y, z;
		y = atan2(-m._13, m._33);
		x = asin(m._23);
		z = atan2(-m._21, m._22);
		return { x, y, z };
	}

	void Rotate(DirectX::XMFLOAT3 rotation)
	{
		DirectX::XMVECTOR quat_rotation = QuaternionRotationYawPitchRoll(rotation.x, rotation.y, rotation.z);
		DirectX::XMVECTOR quat_local = XMLoadFloat4(&localRotationQuat);

		
		XMStoreFloat4(&localRotationQuat, DirectX::XMQuaternionMultiply(quat_local, quat_rotation));
	}

	inline static DirectX::XMFLOAT3 ConvertToRadians(DirectX::XMFLOAT3 degrees) {
		return  {
			DirectX::XMConvertToRadians(degrees.x),
			DirectX::XMConvertToRadians(degrees.y),
			DirectX::XMConvertToRadians(degrees.z)
		};
	}

	inline static DirectX::XMFLOAT3 ConvertToDegrees(DirectX::XMFLOAT3 radians) {
		return {
			DirectX::XMConvertToDegrees(radians.x),
			DirectX::XMConvertToDegrees(radians.y),
			DirectX::XMConvertToDegrees(radians.z)
		};
	}

private:
	DirectX::XMVECTOR QuaternionRotationYawPitchRoll(float Pitch, float Yaw, float Roll) {
		DirectX::XMMATRIX m = DirectX::XMMatrixRotationY(Yaw) * DirectX::XMMatrixRotationX(Pitch) * DirectX::XMMatrixRotationZ(Roll);
		return DirectX::XMQuaternionRotationMatrix(m);
	}
};

}