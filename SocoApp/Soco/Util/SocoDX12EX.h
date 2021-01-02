#pragma once

#include <map>
#include <wrl.h>
#include <Windows.h>
#include "../../Common/d3dx12.h"

//此文件用来拓展DX12一些结构体的功能

namespace Soco {

//作为比较器，能让此作为std::map的Key值
struct BlobPtr_Compare {
	bool operator()(const Microsoft::WRL::ComPtr<ID3DBlob>& lhs, const Microsoft::WRL::ComPtr<ID3DBlob>& rhs) const
	{
		if (lhs->GetBufferSize() < rhs->GetBufferSize()) {
			return true;
		}
		else if (lhs->GetBufferSize() > rhs->GetBufferSize()) {
			return false;
		}

		if (memcmp(lhs->GetBufferPointer(), rhs->GetBufferPointer(), lhs->GetBufferSize()) < 0)
			return true;
		else
			return false;
	}
};
using RootSignatureMap = std::map<Microsoft::WRL::ComPtr<ID3DBlob>, Microsoft::WRL::ComPtr<ID3D12RootSignature>, BlobPtr_Compare>;


//PSO Desc 比较器
//暂时未比较：StreamOutput\NodeMask\CachedPSO

#define COMPARE_LHS_RHS(LHS, RHS) \
	if(LHS < RHS) return true; \
	else if(LHS > RHS) return false;

template <class T>
static int CompareLhsRhsPerByte(const T& lhs, const T& rhs, int offset = 0)
{
	return memcmp((BYTE*)(&lhs) + offset,
					(BYTE*)(&rhs) + offset,
						sizeof(T) - offset);
}
	

struct PsoDesc_Compare {
	bool operator()(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& lhs, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& rhs) const
	{
		COMPARE_LHS_RHS(lhs.pRootSignature, rhs.pRootSignature)

		//VS
		COMPARE_LHS_RHS(lhs.VS.BytecodeLength, rhs.VS.BytecodeLength)
		COMPARE_LHS_RHS(lhs.VS.pShaderBytecode, rhs.VS.pShaderBytecode)

		//PS
		COMPARE_LHS_RHS(lhs.PS.BytecodeLength, rhs.PS.BytecodeLength)
		COMPARE_LHS_RHS(lhs.PS.pShaderBytecode, rhs.PS.pShaderBytecode)

		//DS
		COMPARE_LHS_RHS(lhs.DS.BytecodeLength, rhs.DS.BytecodeLength)
		COMPARE_LHS_RHS(lhs.DS.pShaderBytecode, rhs.DS.pShaderBytecode)

		//HS
		COMPARE_LHS_RHS(lhs.HS.BytecodeLength, rhs.HS.BytecodeLength)
		COMPARE_LHS_RHS(lhs.HS.pShaderBytecode, rhs.HS.pShaderBytecode)

		//GS
		COMPARE_LHS_RHS(lhs.GS.BytecodeLength, rhs.GS.BytecodeLength)
		COMPARE_LHS_RHS(lhs.GS.pShaderBytecode, rhs.GS.pShaderBytecode)

		//BlendState
		int blendStateCompare = CompareLhsRhsPerByte(lhs.BlendState, rhs.BlendState);
		if (blendStateCompare < 0)
			return true;
		else if (blendStateCompare > 0)
			return false;

		//SampleMask
		COMPARE_LHS_RHS(lhs.SampleMask, rhs.SampleMask)

		//RasterizerState
		int rasterizerStateCompare = CompareLhsRhsPerByte(lhs.RasterizerState, rhs.RasterizerState);
		if (rasterizerStateCompare < 0)
			return true;
		else if (rasterizerStateCompare > 0)
			return false;

		//DepthStencilState
		int depthStencilStateCompare = CompareLhsRhsPerByte(lhs.DepthStencilState, rhs.DepthStencilState);
		if (depthStencilStateCompare < 0)
			return true;
		else if (depthStencilStateCompare > 0)
			return false;

		//Input Layout
		COMPARE_LHS_RHS(lhs.InputLayout.NumElements, rhs.InputLayout.NumElements)
		for (int i = 0; i < lhs.InputLayout.NumElements; ++i)
		{
			const D3D12_INPUT_ELEMENT_DESC& lhsIED = lhs.InputLayout.pInputElementDescs[i];
			const D3D12_INPUT_ELEMENT_DESC& rhsIED = rhs.InputLayout.pInputElementDescs[i];

			int lhsIEDNameLength = strlen(lhsIED.SemanticName);
			int rhsIEDNameLength = strlen(rhsIED.SemanticName);

			if (lhsIEDNameLength < rhsIEDNameLength)
				return true;
			else if (lhsIEDNameLength > rhsIEDNameLength)
				return false;

			int nameCompare = memcmp(lhsIED.SemanticName, rhsIED.SemanticName, lhsIEDNameLength);
			if (nameCompare < 0)
				return true;
			else if (nameCompare > 0)
				return false;

			int inputElementDescCompare = CompareLhsRhsPerByte(lhsIED, rhsIED, sizeof(LPCSTR));
			if (inputElementDescCompare < 0)
				return true;
			else if (inputElementDescCompare > 0)
				return false;
		}

		COMPARE_LHS_RHS(lhs.IBStripCutValue, rhs.IBStripCutValue)

		COMPARE_LHS_RHS(lhs.PrimitiveTopologyType, rhs.PrimitiveTopologyType)

		COMPARE_LHS_RHS(lhs.NumRenderTargets, rhs.NumRenderTargets)

		for (int i = 0; i < lhs.NumRenderTargets; ++i)
		{
			COMPARE_LHS_RHS(lhs.RTVFormats[i], rhs.RTVFormats[i]);
		}

		COMPARE_LHS_RHS(lhs.DSVFormat, rhs.DSVFormat)

		int sampleDescCompare = CompareLhsRhsPerByte(lhs.SampleDesc, rhs.SampleDesc);
		if (sampleDescCompare < 0)
			return true;
		else if (sampleDescCompare > 0)
			return false;

		COMPARE_LHS_RHS(lhs.Flags, rhs.Flags)

		return false;
	}
};

using PipelineStateMap = std::map<D3D12_GRAPHICS_PIPELINE_STATE_DESC, Microsoft::WRL::ComPtr<ID3D12PipelineState>, PsoDesc_Compare>;


}