#pragma once
#include "d3dApp.h"
#include <iostream>

struct DescriptorHeapAllocation
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;

	bool IsNull()
	{
		return cpuHandle.ptr == 0 || gpuHandle.ptr == 0;
	}

	DescriptorHeapAllocation()
	{
		cpuHandle.ptr = 0;
		gpuHandle.ptr = 0;
	}
};

class DescriptorHeapAllocator
{
public:
	const UINT MAX_DESCRIPTOR_COUNT;

	//static DescriptorHeapAllocator* GetInstance()
	//{
	//	static DescriptorHeapAllocator* instance = new DescriptorHeapAllocator();
	//	return instance;
	//}

	void Allocate(const UINT NumAllocate = 1, DescriptorHeapAllocation* allocation = nullptr)
	{
		if (AllocatorCount + NumAllocate > MAX_DESCRIPTOR_COUNT) {
			std::cout << "DescriptorHeap已达到最大分配数:" << MAX_DESCRIPTOR_COUNT << std::endl;
			throw std::exception("DescriptorHeap已达到最大分配数");
		}
		else
		{
			if (allocation != nullptr)
			{
				for (int i = 0; i < NumAllocate; ++i)
				{
					CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
					cpuHandle.Offset(AllocatorCount + i, mDescriptorSize);
					CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
					gpuHandle.Offset(AllocatorCount + i, mDescriptorSize);
					allocation[i].cpuHandle = cpuHandle;
					allocation[i].gpuHandle = gpuHandle;
				}
			}

			AllocatorCount += NumAllocate;
		}
	}

	void GetDescriptor(DescriptorHeapAllocation* allocation, const UINT Index)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(Index, mDescriptorSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		gpuHandle.Offset(Index, mDescriptorSize);
		allocation->cpuHandle = cpuHandle;
		allocation->gpuHandle = gpuHandle;
	}

	ID3D12DescriptorHeap* GetDescriptorHeap() { return mDescriptorHeap.Get(); }

	DescriptorHeapAllocator(ID3D12Device* Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, const UINT NumDescriptors, const UINT DescriptorSize)
		:
		MAX_DESCRIPTOR_COUNT(NumDescriptors)
	{
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = MAX_DESCRIPTOR_COUNT;
		srvHeapDesc.Type = Type;
		srvHeapDesc.NodeMask = 0;
		switch (Type)
		{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			break;
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			break;
		}

		ThrowIfFailed(Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mDescriptorHeap)));
	
		mDescriptorSize = DescriptorSize;
	}
private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDescriptorHeap = nullptr;

	int AllocatorCount = 0;
	UINT mDescriptorSize;
};

