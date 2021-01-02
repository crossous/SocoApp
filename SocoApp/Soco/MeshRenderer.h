#pragma once

#include "Renderer.h"

extern const int gNumFrameResources;

namespace Soco
{

class MeshRenderer : public Renderer
{
public:
	MeshRenderer(Material* material, MeshGeometry* geometry,
		const SubmeshGeometry& submesh, const std::string& objCBName = "") : 
		mObjectConstantBufferName(objCBName),
		Renderer(material)
	{
		assert(material != nullptr);
		const D3D12_SHADER_BUFFER_DESC* desc = material->GetConstantBufferDesc(objCBName);
		if (desc != nullptr)
		{
			mPerObjectConstantBufferSize = desc->Size;
			mPerObjectConstantBufferData = std::make_unique<BYTE[]>(desc->Size);
			mResource = std::make_unique<UploadBuffer>(gNumFrameResources, desc->Size, true);
		}
		else
		{
			std::cout << "[Warnning] �޷�����\"" << objCBName << "\"�ҵ�object����������" << std::endl;
		}
		
		mMaterial = material;
		mGeo = geometry;
		//IndexCount = submesh.IndexCount;
		//StartIndexLocation = submesh.StartIndexLocation;
		//BaseVertexLocation = submesh.BaseVertexLocation;
		mSubmeshGeometry = submesh;
	}

	MeshRenderer(MeshRenderer& other) = delete;
	MeshRenderer& operator= (const MeshRenderer& other) = delete;
	MeshRenderer(MeshRenderer&& rhs) :
		Renderer(rhs.mMaterial),
		mNumFramesDirty(rhs.mNumFramesDirty),
		mGeo(rhs.mGeo),
		mPerObjectConstantBufferData(std::move(rhs.mPerObjectConstantBufferData)),
		mObjectConstantBufferName(rhs.mObjectConstantBufferName),
		//mPrimitiveType(rhs.mPrimitiveType),
		//IndexCount(rhs.IndexCount),
		//StartIndexLocation(rhs.StartIndexLocation),
		//BaseVertexLocation(rhs.BaseVertexLocation)
		mSubmeshGeometry(rhs.mSubmeshGeometry)
	{
		rhs.mMaterial = nullptr;
		rhs.mGeo = nullptr;
	}

	//Update To Object
	template <typename T>
	void SetObjectData(T* data)
	{
		mNumFramesDirty = gNumFrameResources;
		memcpy(mPerObjectConstantBufferData.get(), data, sizeof(T));
	}

	void SetObjectData(BYTE* data, UINT offset, UINT size)
	{
		mNumFramesDirty = gNumFrameResources;
		memcpy((BYTE*)mPerObjectConstantBufferData.get() + offset, data, size);
	}

	//no pointer, no reference
	template<typename T>
	std::enable_if_t<std::conjunction_v<std::negation<std::is_reference<T>>, std::negation<std::is_pointer<T>>>, T>
		GetObjectData() {
		return *((T*)mPerObjectConstantBufferData.get());
	}

	//�������ַ����ܼ��ٿ�����������Ҫע�⣺
	//����ǳ�ʼ��������Ľṹ�帳ֵ����SetObjectData���ṹ���������г�ʼֵ
	//������������GetObjectData��ȡ��ַ��ʼ������ʼֵ���Ϊ0
	//���Խ����ʼ����SetObjectData������������GetObjectData<&/*>

	//reference
	template<typename T>
	std::enable_if_t<std::conjunction_v<std::is_reference<T>, std::negation<std::is_pointer<T>>>, T>
		GetObjectData() {
		mNumFramesDirty = gNumFrameResources;
		return *((std::remove_reference_t<T>*)mPerObjectConstantBufferData.get());
	}

	//pointer
	template<typename T>
	std::enable_if_t<std::conjunction_v<std::negation<std::is_reference<T>>, std::is_pointer<T>>, T>
		GetObjectData() {
		mNumFramesDirty = gNumFrameResources;
		return T(mPerObjectConstantBufferData.get());
	}


	//Update To GPU CBuffer
	void Update(int currentFrame) override
	{
		if (mNumFramesDirty > 0 && mPerObjectConstantBufferData != nullptr)
		{
			mResource->CopyData(currentFrame, (BYTE*)mPerObjectConstantBufferData.get(), mPerObjectConstantBufferSize);
			mNumFramesDirty--;
		}
	}

	//Setup RootSignature
	void Setup(ID3D12GraphicsCommandList* cmdList, int currentFrame) override
	{
		cmdList->IASetVertexBuffers(0, 1, &mGeo->VertexBufferView());
		cmdList->IASetIndexBuffer(&mGeo->IndexBufferView());
		//cmdList->IASetPrimitiveTopology(mPrimitiveType);
		mMaterial->SetIASetPrimitiveTopology(cmdList);

		mMaterial->Setup(cmdList, currentFrame);

		if (mPerObjectConstantBufferData != nullptr)
		{
			UINT objectCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(mPerObjectConstantBufferData));
			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mResource->Resource()->GetGPUVirtualAddress() + currentFrame * objectCBByteSize;
			mMaterial->SetGraphicsConstantBufferView(cmdList, mObjectConstantBufferName, objCBAddress);
		}
	}


	void DrawIndexedInstanced(ID3D12GraphicsCommandList* cmdList) override
	{
		cmdList->DrawIndexedInstanced(mSubmeshGeometry.IndexCount, 1, 
			mSubmeshGeometry.StartIndexLocation, mSubmeshGeometry.BaseVertexLocation, 0);
	}


	MeshGeometry* GetGeo() { return mGeo; }
private:
	//ObjectConstants mPerObjectConstantBufferData;

	int mNumFramesDirty = gNumFrameResources;

	MeshGeometry* mGeo = nullptr;
	std::unique_ptr<BYTE[]> mPerObjectConstantBufferData;
	UINT  mPerObjectConstantBufferSize;
	const std::string mObjectConstantBufferName;

	//D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	//UINT IndexCount = 0;
	//UINT StartIndexLocation = 0;
	//int BaseVertexLocation = 0;
	SubmeshGeometry mSubmeshGeometry;

	std::unique_ptr<UploadBuffer> mResource = nullptr;
};
}
