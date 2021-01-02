#include "Material.h"
#include "../Common/d3dApp.h"

namespace Soco{
Material::Material(Shader* shader, MaterialState* drawState, D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc, const std::string& matCBName)
	: mShader(shader), mConstantBufferName(matCBName)
{
	assert(shader != nullptr);
	const D3D12_SHADER_BUFFER_DESC* desc = shader->GetConstantBufferDesc(matCBName);
	if (desc != nullptr) 
	{
		mPerMaterialCBSize = desc->Size;
		mPerMaterialCB = std::make_unique<BYTE[]>(desc->Size);
		mResource = std::make_unique<UploadBuffer>(gNumFrameResources, desc->Size, true);
	}

	//if (drawState == nullptr)
	//{ 
	//	mDrawState.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//	mDrawState.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//	mDrawState.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//}
	//else
	//{
	//	mDrawState = *drawState;
	//}

	BuildPSODesc(psoDesc, drawState);
	const std::map<std::string, UINT>& textureSlot = mShader->GetTextureSlot();
	for (auto ite = textureSlot.cbegin(); ite != textureSlot.cend(); ++ite)
	{
		mTexture[ite->first] = nullptr;
	}
}

void Material::BuildPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc, MaterialState* drawState)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC tempPSODesc;

	if (psoDesc == nullptr)
	{
		psoDesc = &tempPSODesc;
		SetDefaultPSOData(&tempPSODesc);
	}

	mShader->SetPSODescRootSignature(psoDesc);
	mShader->SetPSODescShader(psoDesc);
	mShader->SetPSODescInputLayout(psoDesc);
	mShader->SetPSODescTopology(psoDesc);

	if (drawState != nullptr)
	{
		psoDesc->RasterizerState = drawState->rasterizeState;
		psoDesc->BlendState = drawState->blendState;
		psoDesc->DepthStencilState = drawState->depthStencilState;
	}
	else
	{
		psoDesc->RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc->BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc->DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	}


	memcpy(&mPSODesc, psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	mPSO = PipelineStateManager::GetInstance()->GetPipelineState(D3DApp::GetDevice(), psoDesc);
}

void Material::SetDefaultPSOData(D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc)
{
	ZeroMemory(psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc->SampleMask = UINT_MAX;
	//psoDesc->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc->NumRenderTargets = 1;

	psoDesc->RTVFormats[0] = D3DApp::GetBackBufferFormat();
	psoDesc->DSVFormat = D3DApp::GetDepthStencilFormat();
	auto[MsaaState, MsaaQuality] = D3DApp::GetMSAAState();
	psoDesc->SampleDesc.Count = MsaaState ? 4 : 1;
	psoDesc->SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
}

void Material::Update(int currentFrame)
{
	if (mPerMaterialCB == nullptr)
		return;

	if (mNumFrameDirty > 0)
	{
		mResource->CopyData(currentFrame, mPerMaterialCB.get(), mPerMaterialCBSize);
		mNumFrameDirty--;
	}
}

void Material::SetTexture(const std::string& name, Texture* texture)
{
	if (mTexture.find(name) != mTexture.end())
	{
		mTexture[name] = texture;
	}
	else
	{
		std::cout << "当前Material的Shader没有名为" << name << "的Texture" << std::endl;
		//throw std::exception("当前Material的Shader没有此材质");
	}
}

void Material::Setup(ID3D12GraphicsCommandList* cmdList, int currentFrame)
{
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(mPerMaterialCBSize);

	if (mResource != nullptr)
	{
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = mResource->Resource()->GetGPUVirtualAddress() + currentFrame * matCBByteSize;
		mShader->SetConstantBufferView(cmdList, mConstantBufferName, matCBAddress);
	}

	for (auto ite = mTexture.begin(); ite != mTexture.end(); ++ite)
	{
		if (ite->second != nullptr)
		{
			mShader->SetTexture(cmdList, ite->first, ite->second->SRV());
		}
	}
}

void Material::SetRasterizerState(D3D12_RASTERIZER_DESC& rasterizeState)
{
	mPSODesc.RasterizerState = rasterizeState;
	mPSO = mPSO = PipelineStateManager::GetInstance()->GetPipelineState(D3DApp::GetDevice(), &mPSODesc);
}

void Material::SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC& depthStencilState)
{
	mPSODesc.DepthStencilState = depthStencilState;
	mPSO = mPSO = PipelineStateManager::GetInstance()->GetPipelineState(D3DApp::GetDevice(), &mPSODesc);
}
void Material::SetBlendStateState(D3D12_BLEND_DESC& blendState)
{
	mPSODesc.BlendState = blendState;
	mPSO = mPSO = PipelineStateManager::GetInstance()->GetPipelineState(D3DApp::GetDevice(), &mPSODesc);
}
void Material::SetMaterialState(MaterialState& drawState)
{
	mPSODesc.RasterizerState = drawState.rasterizeState;
	mPSODesc.DepthStencilState = drawState.depthStencilState;
	mPSODesc.BlendState = drawState.blendState;

	mPSO = mPSO = PipelineStateManager::GetInstance()->GetPipelineState(D3DApp::GetDevice(), &mPSODesc);
}

MaterialState Material::GetMaterialState()
{
	MaterialState res;
	res.rasterizeState = mPSODesc.RasterizerState;
	res.depthStencilState = mPSODesc.DepthStencilState;
	res.blendState = mPSODesc.BlendState;
	return res;
}

bool Material::PipelineStateEqual(const Material& lhs, const Material& rhs)
{
	return lhs.mPSO == rhs.mPSO;
}

bool Material::RootSignatureEqual(const Material& lhs, const Material& rhs)
{
	return Shader::RootSignatureEqual(*(lhs.mShader), *(rhs.mShader));
}

}