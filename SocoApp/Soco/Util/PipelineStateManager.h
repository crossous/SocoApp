#pragma once

#include "SocoDX12EX.h"
#include "../../Common/d3dUtil.h"
#include <iostream>

namespace Soco 
{
class PipelineStateManager {
public:
	static PipelineStateManager* GetInstance() {
		static PipelineStateManager* instance = new PipelineStateManager();
		return instance;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc)
	{
		if (mPipelineStateMap.find(*desc) == mPipelineStateMap.end())
		{
			ThrowIfFailed(device->CreateGraphicsPipelineState(desc, IID_PPV_ARGS(&mPipelineStateMap[*desc])));
		}
		return mPipelineStateMap[*desc];
	}

private:
	PipelineStateMap mPipelineStateMap;
};

}