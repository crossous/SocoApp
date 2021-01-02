#include "Terrain.h"
#include <iostream>
#include "../Common/DescriptorHeapAllocator.h"
#include "Util/PrintHelper.h"

#include <algorithm>

namespace Soco
{
Terrain::Terrain(const char* HeightMapFilename)
{
	LoadHeightMap(HeightMapFilename);
	BuildTerrainMesh();
}

void Terrain::LoadHeightMap(const char* HeightMapFilename)
{
	UINT width, height;
	unsigned char* data;

	UINT error = lodepng_decode32_file(&data, &width, &height, HeightMapFilename);
	if (error)
	{
		std::cout << "高度图" << HeightMapFilename << "加载错误" << std::endl;
		throw std::exception();
	}

	mHeightMapData = std::unique_ptr<unsigned char>(data);

	D3D12_RESOURCE_DESC descTex = {};
	descTex.MipLevels = 1;
	descTex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descTex.Width = width;
	descTex.Height = height;
	descTex.Flags = D3D12_RESOURCE_FLAG_NONE;
	descTex.DepthOrArraySize = 1;
	descTex.SampleDesc.Count = 1;
	descTex.SampleDesc.Quality = 0;
	descTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	ThrowIfFailed(D3DApp::GetApp()->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&descTex,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(mHeightMapResource.GetAddressOf())
	));
	mHeightMapResource->SetName(L"Height Map");

	D3D12_SUBRESOURCE_DATA dataTex = {};
	dataTex.pData = data;
	dataTex.RowPitch = width * 4 * sizeof(unsigned char);
	dataTex.SlicePitch = height * width * 4 * sizeof(unsigned char);

	const UINT num2DSubresources = descTex.MipLevels * descTex.DepthOrArraySize;

	auto size = GetRequiredIntermediateSize(mHeightMapResource.Get(), 0, num2DSubresources);
	size = (UINT64)pow(2.0, ceil(log(size) / log(2)));
	ThrowIfFailed(D3DApp::GetApp()->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mUploadBuffer.GetAddressOf())
	));

	mUploadBuffer->SetName(L"Height Map Upload Buffer");
	UpdateSubresources(D3DApp::GetApp()->GetCommandList(), mHeightMapResource.Get(), mUploadBuffer.Get(), 0, 0, num2DSubresources, &dataTex);
	D3DApp::GetApp()->GetCommandList()->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(mHeightMapResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	);

	DescriptorHeapAllocation allocation;
	D3DApp::GetCbvSrvUavAllocate(&allocation, 1);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = descTex.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = descTex.MipLevels;

	D3DApp::GetApp()->GetDevice()->CreateShaderResourceView(mHeightMapResource.Get(), &srvDesc, allocation.cpuHandle);
	mSrv = allocation.gpuHandle;

	mTexture = std::make_unique<TerrainTexture>(mHeightMapResource, mUploadBuffer, mSrv);

	//for (int i = 0; i < mTexture->Height(); ++i)
	//{
	//	for (int j = 0; j < mTexture->Width(); ++j)
	//	{
	//		DirectX::XMFLOAT4 Color = GetHeightMapValue({ j, i });
	//		std::cout << Color.x << ", " << Color.y << ", " << Color.z << std::endl;
	//	}
	//}
}

void Terrain::BuildTerrainMesh()
{

	//GeometryGenerator geoGen;
	//GeometryGenerator::MeshData grid = geoGen.CreateTerrain(mTexture->Width(), mTexture->Height(), mTexture->Width() / 16, mTexture->Height() / 16);
	
	float MeshDownScale = 16;//
	//float HeightScale = 50;//高度缩放

	float width = mTexture->Width(), height = mTexture->Height();
	float m = width / MeshDownScale, n = height / MeshDownScale;

	uint32_t vertexCount = m * n;
	uint32_t quadFaceCount = (m - 1) * (n - 1);

	float halfWidth = 0.5f * width;
	float halfHeight = 0.5f * height;

	float dx = width / (m - 1);
	float dz = height / (n - 1);

	float du = 1.0f / (m - 1);
	float dv = 1.0f / (n - 1);

	std::vector<TerrainVertex> vertices(vertexCount);
	for (uint32_t i = 0; i < n; ++i)
	{
		float z = -halfHeight + i * dz;
		for (uint32_t j = 0; j < m; ++j)
		{
			float x = -halfWidth + j * dx;

			float height = GetHeightMapValue({ static_cast<int>(j * MeshDownScale), static_cast<int>(i * MeshDownScale) }).y * mHeightScale;

			vertices[i*n + j].position = DirectX::XMFLOAT3(x, height, z);

			vertices[i*n + j].uv.x = j * du;
			vertices[i*n + j].uv.y = i * dv;
		}
	}

	std::vector<uint32_t> indices(quadFaceCount * 4);

	uint32_t k = 0;
	for (uint32_t i = 0; i < n - 1; ++i)
	{
		for (uint32_t j = 0; j < m - 1; ++j)
		{

			indices[k] = (i + 1) * n + j;
			indices[k + 1] = (i + 1) * n + j + 1;
			indices[k + 2] = i * n + j;
			indices[k + 3] = i * n + j + 1;

			k += 4;
		}
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TerrainVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "TerrainGeo";

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(D3DApp::GetDevice(),
		D3DApp::GetCommandList(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->VertexBufferGPU->SetName(L"Terrain Vertex Buffer");
	geo->VertexBufferUploader->SetName(L"Terrain Vertex Buffer Uploader");

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(D3DApp::GetDevice(),
		D3DApp::GetCommandList(), indices.data(), ibByteSize, geo->IndexBufferUploader);
	geo->IndexBufferGPU->SetName(L"Terrain Index Buffer");
	geo->IndexBufferUploader->SetName(L"Terrain Index Buffer Uploader");

	geo->VertexByteStride = sizeof(TerrainVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs[mSubmeshName] = submesh;

	mGeo = std::move(geo);
}

DirectX::XMFLOAT4 Terrain::GetHeightMapValue(DirectX::XMINT2 index)
{
	const int DEPTH = 4;
	unsigned char* data = mHeightMapData.get();
	int x = std::clamp(index.x, 0, static_cast<std::int32_t>(mTexture->Width() - 1));
	int y = std::clamp(index.y, 0, static_cast<std::int32_t>(mTexture->Height() - 1));

	unsigned char* target = data + (y * mTexture->Width() + x) * DEPTH;
	return { (float)*target / 255.0f, (float)*(target + 1) / 255.0f, (float)*(target + 2) / 255.0f, (float)*(target + 3) / 255.0f };
}
}