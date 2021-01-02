#pragma once

#include "Shader.h"
#include "Texture.h"
#include "../Common/UploadBuffer.h"
#include "Util/PipelineStateManager.h"
#include <memory>

extern const int gNumFrameResources;
namespace Soco {


struct MaterialState {
	D3D12_RASTERIZER_DESC rasterizeState;
	D3D12_DEPTH_STENCIL_DESC depthStencilState;
	D3D12_BLEND_DESC blendState;
};

class Material {
public:


public:

	Material(Shader* shader, MaterialState* drawState = nullptr, D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc = nullptr, const std::string& matCBName = "");
	Material(Material& other) = delete;
	Material& operator= (const Material& other) = delete;
	Material(Material&& rhs)
		:
		mConstantBufferName(rhs.mConstantBufferName),
		mShader(rhs.mShader),
		//mDrawState(rhs.mDrawState),
		mPerMaterialCB(std::move(rhs.mPerMaterialCB)),
		mPerMaterialCBSize(rhs.mPerMaterialCBSize),
		mPSO(std::move(rhs.mPSO)),
		mNumFrameDirty(rhs.mNumFrameDirty),
		mResource(std::move(rhs.mResource)),
		mTexture(std::move(rhs.mTexture)),
		mPSODesc(rhs.mPSODesc)
	{ 
		rhs.mShader = nullptr;
		rhs.mPerMaterialCBSize = 0;
		rhs.mPSO = nullptr;
		rhs.mNumFrameDirty = 0;
		ZeroMemory(&rhs.mPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	}

	~Material() {};

	template<typename T>
	void SetMaterialData(T* data) {
		mNumFrameDirty = gNumFrameResources;
		memcpy(mPerMaterialCB.get(), data, sizeof(T));
	}

	void SetMaterialData(BYTE* data, UINT offset, UINT size) {
		mNumFrameDirty = gNumFrameResources;
		memcpy(mPerMaterialCB.get() + offset, data, size);
	}
	//no pointer, no reference
	template<typename T>
	std::enable_if_t<std::conjunction_v<std::negation<std::is_reference<T>>, std::negation<std::is_pointer<T>>>, T> 
	GetMaterialData() {
		return *((T*)mPerMaterialCB.get());
	}
	
	//reference
	template<typename T>
	std::enable_if_t<std::conjunction_v<std::is_reference<T>, std::negation<std::is_pointer<T>>>, T>
	GetMaterialData(){
		mNumFrameDirty = gNumFrameResources;
		return *((std::remove_reference_t<T>*)mPerMaterialCB.get());
	}

	//pointer
	template<typename T>
	std::enable_if_t<std::conjunction_v<std::negation<std::is_reference<T>>, std::is_pointer<T>>, T>
	GetMaterialData() {
		mNumFrameDirty = gNumFrameResources;
		return T(mPerMaterialCB.get());
	}

	void Update(int currentFrame);
	void SetTexture(const std::string& name, Texture* texture);
	void Setup(ID3D12GraphicsCommandList* cmdList, int currentFrame);


	static bool PipelineStateEqual(const Material& lhs, const Material& rhs);
	static bool RootSignatureEqual(const Material& lhs, const Material& rhs);

	void SetGraphicsRootSignature(ID3D12GraphicsCommandList* cmdList) { mShader->SetGraphicsRootSignature(cmdList); }
	void SetGraphicsConstantBufferView(ID3D12GraphicsCommandList* cmdList, const std::string& variableName, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation)
	{
		mShader->SetConstantBufferView(cmdList, variableName, BufferLocation);
	}

	void SetPipelineState(ID3D12GraphicsCommandList* cmdList) { cmdList->SetPipelineState(mPSO.Get()); }

	const D3D12_SHADER_BUFFER_DESC* GetConstantBufferDesc(const std::string& name) { return mShader->GetConstantBufferDesc(name); }

	void SetIASetPrimitiveTopology(ID3D12GraphicsCommandList* cmdList)
	{
		mShader->SetIASetPrimitiveTopology(cmdList);
	}

public:
	void SetRasterizerState(D3D12_RASTERIZER_DESC& rasterizeState);
	void SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC& depthStencilState);
	void SetBlendStateState(D3D12_BLEND_DESC& blendState);
	void SetMaterialState(MaterialState& drawState);
	MaterialState GetMaterialState();

private:
	void BuildPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc, MaterialState* drawState);
	void SetDefaultPSOData(D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc);

private:
	const std::string mConstantBufferName;
	Shader* mShader = nullptr;
	std::unique_ptr<BYTE[]> mPerMaterialCB = nullptr;
	UINT mPerMaterialCBSize = 0;
	//MaterialState mDrawState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO;
	int mNumFrameDirty = gNumFrameResources;
	std::unique_ptr<UploadBuffer> mResource = nullptr;
	std::map<std::string, Texture*> mTexture;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC mPSODesc;
};
}