#pragma once
#include "../Common/lodepng.h"
#include "../Common/d3dUtil.h"
#include "../Common/d3dApp.h"
#include "../Common/GeometryGenerator.h"

#include "Texture.h"

// 非自由限制：
// 输入布局、Shader、Material、Mesh


namespace Soco
{
class TerrainRenderer;

class Terrain
{
public:
	struct TerrainVertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 uv;
	};

	class TerrainTexture : public Texture
	{
	public:
		TerrainTexture(Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer, CD3DX12_GPU_DESCRIPTOR_HANDLE& gpuHandle)
			: Texture(resource, uploadBuffer, gpuHandle) {
		}

	private:
		virtual void CreateSRV() override {}
	};

public:
	Terrain(const char* HeightMapFilename);

	TerrainTexture* GetTexture() { return mTexture.get(); }
	MeshGeometry* GetMesh() { return mGeo.get(); }
	SubmeshGeometry* GetSubmesh() { return &(mGeo->DrawArgs[mSubmeshName]); }
	float GetHeightScale() { return mHeightScale; }

private:
	void LoadHeightMap(const char* HeightMapFilename);
	void BuildTerrainMesh();

	DirectX::XMFLOAT4 GetHeightMapValue(DirectX::XMINT2);



private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mHeightMapResource;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mSrv;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	std::unique_ptr<unsigned char> mHeightMapData;
	std::unique_ptr<TerrainTexture> mTexture;

	std::unique_ptr<MeshGeometry> mGeo;
	const std::string mSubmeshName = "grid";
	const float mHeightScale = 200;

};
}

