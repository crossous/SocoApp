#pragma once
#include <string>
#include "../Common/d3dUtil.h"
#include "../Common/d3dApp.h"
//#include "DescriptorHeapManager/DescriptorHeapAllocator.h"

namespace Soco 
{
class Texture
{
public:
	const std::string Name;
	const std::wstring Filename;

	ID3D12Resource* GetResource()
	{
		return Resource.Get();
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE SRV() 
	{ 
		if (!mSrvCreated)
		{
			CreateSRV();
			mSrvCreated = true;
		}
			
		return mGpuHandle;
	}
	UINT Width() { return Resource->GetDesc().Width; }
	UINT Height() { return Resource->GetDesc().Height; }

	virtual ~Texture() {}

protected:
	Texture(const std::string name, const std::wstring filename)
		: Name(name), Filename(filename)
	{ 
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(D3DApp::GetApp()->GetDevice(), D3DApp::GetApp()->GetCommandList(),
			Filename.c_str(), Resource, UploadHeap));
		Resource->SetName(filename.c_str());
	}

	Texture(Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer, CD3DX12_GPU_DESCRIPTOR_HANDLE& gpuHandle)
	{
		Resource = resource;
		UploadHeap = uploadBuffer;
		mGpuHandle = gpuHandle;
	}

	//必须保证在初始化资源后，调用DelayInit
	Texture() {}

	virtual void CreateSRV() = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuHandle;

	bool mSrvCreated = false;
private:

};

//Texture2D仅作为Shader Resource
class Texture2D : public Texture
{
public:
	Texture2D(const std::string& name, const std::wstring& filename)
		: Texture(name, filename)
	{}

private:
	virtual void CreateSRV() override
	{
		DescriptorHeapAllocation allocation;
		D3DApp::GetCbvSrvUavAllocate(&allocation, 1);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = Resource->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;

		D3DApp::GetDevice()->CreateShaderResourceView(Resource.Get(), &srvDesc, allocation.cpuHandle);
		mGpuHandle = allocation.gpuHandle;
	}
};

class TextureCube : public Texture
{
public:
	TextureCube(const std::string& name, const std::wstring& filename)
		: Texture(name, filename)
	{}

private:
	virtual void CreateSRV() override
	{
		DescriptorHeapAllocation allocation;
		D3DApp::GetCbvSrvUavAllocate(&allocation, 1);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = Resource->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = Resource->GetDesc().MipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		D3DApp::GetDevice()->CreateShaderResourceView(Resource.Get(), &srvDesc, allocation.cpuHandle);
		mGpuHandle = allocation.gpuHandle;
	}
};

/***
DsvFormat DSV格式，不能与Rtv格式、UAV格式一起设置
*/


struct RenderTextureFormat
{
	DXGI_FORMAT ResourceFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT SrvFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT UavFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT RtvFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT DsvFormat = DXGI_FORMAT_UNKNOWN;
};

class RenderTexture : public Texture
{
public:
	RenderTexture(UINT Width, UINT Height, RenderTextureFormat& Formats) : mViewFormats(Formats)
	{
		assert(Formats.ResourceFormat != DXGI_FORMAT_UNKNOWN && "资源格式需要设置");
		assert(Formats.SrvFormat != DXGI_FORMAT_UNKNOWN && "SRV格式需要设置");
		if (Formats.DsvFormat != DXGI_FORMAT_UNKNOWN && (Formats.RtvFormat != DXGI_FORMAT_UNKNOWN || Formats.UavFormat != DXGI_FORMAT_UNKNOWN))
		{
			std::cout << "DsvFormat DSV格式，不能与Rtv格式、UAV格式一起设置" << std::endl;
			assert(0);
		}

		BuildResource(Width, Height);
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE UAV()
	{
		if (!mUavCreate)
		{
			CreateUAV();
			mUavCreate = true;
		}

		return mUavAllocation.gpuHandle;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE RTV()
	{
		if (!mRtvCreate)
		{
			CreateRTV();
			mRtvCreate = true;
		}

		return mRtvAllocation.cpuHandle;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE DSV()
	{
		if (!mDsvCreate)
		{
			CreateDsv();
			mDsvCreate = true;
		}

		return mDsvAllocation.cpuHandle;
	}

	void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES nextState)
	{
		if (nextState != mCurrState)
		{
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Resource.Get(),
				mCurrState, nextState));
			mCurrState = nextState;
		}
	}

	void Resize(UINT newWidth, UINT newHeight)
	{
		if (newWidth != Resource->GetDesc().Width || newHeight != Resource->GetDesc().Height)
		{
			BuildResource(newWidth, newHeight);
			mSrvCreated = false;
			mUavCreate = false;
			mRtvCreate = false;
			mDsvCreate = false;
		}
	}

private:
	void BuildResource(UINT Width, UINT Height)
	{
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = Width;
		texDesc.Height = Height;
		texDesc.DepthOrArraySize = 1;
		texDesc.MipLevels = 1;
		texDesc.Format = mViewFormats.ResourceFormat;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (mViewFormats.UavFormat != DXGI_FORMAT_UNKNOWN)
		{
			texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (mViewFormats.RtvFormat != DXGI_FORMAT_UNKNOWN)
		{
			texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (mViewFormats.DsvFormat != DXGI_FORMAT_UNKNOWN)
		{
			texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}

		ThrowIfFailed(D3DApp::GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&Resource)
		));

		mCurrState = D3D12_RESOURCE_STATE_COMMON;
	}


	virtual void CreateSRV() override
	{
		if(mSrvAllocation.IsNull())
			D3DApp::GetCbvSrvUavAllocate(&mSrvAllocation, 1);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = mViewFormats.SrvFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;

		D3DApp::GetDevice()->CreateShaderResourceView(Resource.Get(), &srvDesc, mSrvAllocation.cpuHandle);
		mGpuHandle = mSrvAllocation.gpuHandle;
	}

	void CreateUAV()
	{
		assert(mViewFormats.UavFormat != DXGI_FORMAT_UNKNOWN && "创建UAV的纹理格式不能为DXGI_FORMAT_UNKNOWN");
		if(mUavAllocation.IsNull())
			D3DApp::GetCbvSrvUavAllocate(&mUavAllocation, 1);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

		uavDesc.Format = mViewFormats.UavFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		D3DApp::GetDevice()->CreateUnorderedAccessView(Resource.Get(), nullptr, &uavDesc, mUavAllocation.cpuHandle);
	}

	void CreateRTV()
	{
		assert(mViewFormats.RtvFormat != DXGI_FORMAT_UNKNOWN && "创建RTV的纹理格式不能为DXGI_FORMAT_UNKNOWN");
		if(mRtvAllocation.IsNull())
			D3DApp::GetRtvAllocate(&mRtvAllocation, 1);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};

		rtvDesc.Format = mViewFormats.RtvFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		D3DApp::GetDevice()->CreateRenderTargetView(Resource.Get(), &rtvDesc, mRtvAllocation.cpuHandle);
	}

	void CreateDsv()
	{
		assert(mViewFormats.DsvFormat != DXGI_FORMAT_UNKNOWN && "创建DSV的纹理格式不能为DXGI_FORMAT_UNKNOWN");
		if(mDsvAllocation.IsNull())
			D3DApp::GetDsvAllocate(&mDsvAllocation, 1);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = mViewFormats.DsvFormat;
		dsvDesc.Texture2D.MipSlice = 0;

		D3DApp::GetDevice()->CreateDepthStencilView(Resource.Get(), &dsvDesc, mDsvAllocation.cpuHandle);
	}


private:
	D3D12_RESOURCE_STATES mCurrState = D3D12_RESOURCE_STATE_COMMON;
	bool mUavCreate = false;
	bool mRtvCreate = false;
	bool mDsvCreate = false;

	RenderTextureFormat mViewFormats;

	DescriptorHeapAllocation mSrvAllocation;
	DescriptorHeapAllocation mUavAllocation;
	union {
		DescriptorHeapAllocation mRtvAllocation;
		DescriptorHeapAllocation mDsvAllocation;
	};
};

}


