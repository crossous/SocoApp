#pragma once
#include "Renderer.h"
#include "Util/PipelineStateManager.h"

extern const int gNumFrameResources;

namespace Soco
{
class TextureRenderer : public Renderer
{
public:
	struct TexPositionCB
	{
		DirectX::XMFLOAT2 size = {1, 1};
		DirectX::XMFLOAT2 offset = {0, 0};
	};


	TextureRenderer(Texture* texture, DirectX::XMFLOAT2 size = {1, 1}, DirectX::XMFLOAT2 offset = {0, 0})
		:Renderer(nullptr), mTexture(texture)
	{
		Soco::ShaderStage ss;
		ss.vs = "VS";
		ss.ps = "PS";

		mShader = std::make_unique<Shader>(L"Shaders\\Draw2D.hlsl", nullptr, ss);
		//mTextureSlot = mShader->GetSlot("MainTex");
		BuildPSODesc();
		BuildResource();

		mTexPositionCB.size = size;
		mTexPositionCB.offset = offset;
	}

	void SetTexture(Texture* texture)
	{
		mTexture = texture;
	}

	TexPositionCB& GetTexPosCB()
	{
		mNumFramesDirty = gNumFrameResources;
		return mTexPositionCB;
	}

	void Update(int currentFrame)
	{
		if (mResource != nullptr && mNumFramesDirty > 0)
		{
			mResource->CopyData(currentFrame, (BYTE*)&mTexPositionCB, sizeof(TexPositionCB));
			mNumFramesDirty--;
		}
	}

	void Setup(ID3D12GraphicsCommandList* cmdList, int currnetFrame)override
	{
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		mShader->SetTexture(cmdList, "MainTex", mTexture->SRV());

		if (mResource != nullptr)
		{
			UINT texPosCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(TexPositionCB));
			D3D12_GPU_VIRTUAL_ADDRESS texPosCBAddress = mResource->Resource()->GetGPUVirtualAddress() + currnetFrame * texPosCBByteSize;
			mShader->SetConstantBufferView(cmdList, mTexPositionCBName, texPosCBAddress);
		}
		else
		{
			std::cout << ">?";
		}

	}

	void DrawIndexedInstanced(ID3D12GraphicsCommandList* cmdList)
	{
		cmdList->DrawInstanced(4, 1, 0, 0);
	}


	void SetGraphicsRootSignature(ID3D12GraphicsCommandList* cmdList)override 
	{
		mShader->SetGraphicsRootSignature(cmdList);
	}
	void SetPipelineState(ID3D12GraphicsCommandList* cmdList)override
	{
		cmdList->SetPipelineState(mPSO.Get());
	}
	void SetCBV(ID3D12GraphicsCommandList* cmdList, const std::string& cbName, D3D12_GPU_VIRTUAL_ADDRESS address) override
	{
		mShader->SetConstantBufferView(cmdList, cbName, address);
	}

private:
	void BuildPSODesc()
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC tempPSODesc;
		ZeroMemory(&tempPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		tempPSODesc.SampleMask = UINT_MAX;
		//tempPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		tempPSODesc.NumRenderTargets = 1;

		tempPSODesc.RTVFormats[0] = D3DApp::GetBackBufferFormat();
		tempPSODesc.DSVFormat = D3DApp::GetDepthStencilFormat();
		auto[MsaaState, MsaaQuality] = D3DApp::GetMSAAState();
		tempPSODesc.SampleDesc.Count = MsaaState ? 4 : 1;
		tempPSODesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;

		tempPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		tempPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		tempPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		tempPSODesc.DepthStencilState.DepthEnable = FALSE;
		tempPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		tempPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		mShader->SetPSODescRootSignature(&tempPSODesc);
		mShader->SetPSODescShader(&tempPSODesc);
		mShader->SetPSODescInputLayout(&tempPSODesc);
		mShader->SetPSODescTopology(&tempPSODesc);

		mPSO = PipelineStateManager::GetInstance()->GetPipelineState(D3DApp::GetDevice(), &tempPSODesc);
	}
	void BuildResource()
	{
		const D3D12_SHADER_BUFFER_DESC* desc = mShader->GetConstantBufferDesc(mTexPositionCBName);
		if (desc != nullptr)
		{
			assert(desc->Size == sizeof(TexPositionCB));
			mResource = std::make_unique<UploadBuffer>(gNumFrameResources, desc->Size, true);
		}
	}


private:
	int mNumFramesDirty = gNumFrameResources;
	const char* mTexPositionCBName = "cbTexPosition";

	Texture* mTexture;
	std::unique_ptr<Shader> mShader;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO;
	std::unique_ptr<UploadBuffer> mResource = nullptr;
	TexPositionCB mTexPositionCB;

};
}


