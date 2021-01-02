#pragma once

#include <string>
#include <map>
#include <vector>
#include "../Common/d3dUtil.h"
#include "../Common/UploadBuffer.h"
#include "Material.h"

#include "Util/PrintHelper.h"

namespace Soco
{
class Renderer
{
protected:
	Renderer(Material* material) : mMaterial(material){}
	Renderer(Renderer& other) = delete;
	Renderer& operator= (const Renderer& other) = delete;
	Renderer(Renderer&& rhs)
		: mMaterial(rhs.mMaterial) 
	{
		rhs.mMaterial = nullptr;
	}

protected:
	Material* mMaterial = nullptr;

public:
	virtual void Update(int currentFrame){};
	virtual void Setup(ID3D12GraphicsCommandList* cmdList, int currnetFrame) = 0;
	virtual void DrawIndexedInstanced(ID3D12GraphicsCommandList* cmdList) = 0;

	virtual void SetGraphicsRootSignature(ID3D12GraphicsCommandList* cmdList) { mMaterial->SetGraphicsRootSignature(cmdList); }
	virtual void SetPipelineState(ID3D12GraphicsCommandList* cmdList) { mMaterial->SetPipelineState(cmdList); }
	virtual void SetCBV(ID3D12GraphicsCommandList* cmdList, const std::string& cbName, D3D12_GPU_VIRTUAL_ADDRESS address)
	{
		mMaterial->SetGraphicsConstantBufferView(cmdList, cbName, address);
	}

	virtual ~Renderer(){}
};
}
