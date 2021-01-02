#pragma once

#include "Renderer.h"

namespace Soco
{

class SkyboxRenderer : public Renderer
{
private:
	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 TexC;
	};

public:
	SkyboxRenderer(Material* material, const std::string& CubeMapName = "gCubeMap") :
		Renderer(material)
	{
		assert(material != nullptr);
		BuildBox();
		mMaterial = material;
	}

	SkyboxRenderer(Material* material, std::unique_ptr<MeshGeometry>& geometry, const std::string submeshName, const std::string& CubeMapName = "gCubeMap") :
		Renderer(material)
	{
		assert(material != nullptr);
		mSkyboxMesh = std::move(geometry);
		mSubmeshName = submeshName;
		mMaterial = material;
	}

	void Setup(ID3D12GraphicsCommandList* cmdList, int currentFrame) override
	{
		cmdList->IASetVertexBuffers(0, 1, &mSkyboxMesh->VertexBufferView());
		cmdList->IASetIndexBuffer(&mSkyboxMesh->IndexBufferView());
		cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		mMaterial->Setup(cmdList, currentFrame);
	}

	void DrawIndexedInstanced(ID3D12GraphicsCommandList* cmdList) override
	{
		SubmeshGeometry& submesh = mSkyboxMesh->DrawArgs[mSubmeshName];
		cmdList->DrawIndexedInstanced(submesh.IndexCount, 1, submesh.StartIndexLocation, submesh.BaseVertexLocation, 0);
	}

private:
	//构造默认网格数据
	//需要传入的Shader与InputLayout对应，否则需要手动传入Mesh
	void BuildBox()
	{
		GeometryGenerator geoGen;
		//[-1, 1]x[-1, 1]x[-1, 1]
		//ndc下相机的四点
		GeometryGenerator::MeshData box = geoGen.CreateBox(2.0f, 2.0f, 2.0f, 0);

		std::vector<Vertex> vertices(box.Vertices.size());
		for (size_t i = 0; i < box.Vertices.size(); ++i)
		{
			auto& p = box.Vertices[i].Position;
			vertices[i].Pos = p;
			vertices[i].Normal = box.Vertices[i].Normal;
			vertices[i].TexC = box.Vertices[i].TexC;
		}

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

		std::vector<std::uint16_t> indices = box.GetIndices16();
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "boxGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(D3DApp::GetApp()->GetDevice(),
			D3DApp::GetApp()->GetCommandList(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
		geo->VertexBufferGPU->SetName(L"Skybox Vertex Buffer");
		geo->VertexBufferUploader->SetName(L"Skybox Vertex Buffer Uploader");

		geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(D3DApp::GetApp()->GetDevice(),
			D3DApp::GetApp()->GetCommandList(), indices.data(), ibByteSize, geo->IndexBufferUploader);
		geo->IndexBufferGPU->SetName(L"Skybox Index Buffer");
		geo->IndexBufferUploader->SetName(L"Skybox Index Buffer Uploader");

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs["box"] = submesh;

		mSkyboxMesh = std::move(geo);
		mSubmeshName = "box";
	}

private:
	std::unique_ptr<MeshGeometry> mSkyboxMesh;
	std::string mSubmeshName;
};

}
