#pragma once

#include "SocoDX12EX.h"
#include "../../Common/d3dUtil.h"
#include "../../Common/d3dApp.h"
#include <iostream>

namespace Soco 
{
class RootSignatureManager {
public:
	static RootSignatureManager* GetInstance() {
		static RootSignatureManager* instance = new RootSignatureManager();
		return instance;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature(Microsoft::WRL::ComPtr<ID3DBlob> serializeRootSignature) {
		if (mRootSignatureMap.find(serializeRootSignature) == mRootSignatureMap.end()) {
			ThrowIfFailed(D3DApp::GetApp()->GetDevice()->CreateRootSignature(
				0,
				serializeRootSignature->GetBufferPointer(),
				serializeRootSignature->GetBufferSize(),
				IID_PPV_ARGS(mRootSignatureMap[serializeRootSignature].GetAddressOf())
				));
		}

		return mRootSignatureMap[serializeRootSignature];
	}

private:
	RootSignatureMap mRootSignatureMap;

};
}