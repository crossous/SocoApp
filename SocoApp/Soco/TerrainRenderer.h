#pragma once

#include "MeshRenderer.h"
#include "Terrain.h"

//依赖梳理
//准备的顶点数据，需要通过Shader布局

namespace Soco
{
struct TerrainObjectConstants
{
	//DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	float texWidth;
	float texHeight;
	float height;
};

class TerrainRenderer : public MeshRenderer
{
public:
	TerrainRenderer(Terrain* terrain, Material* material, const std::string& objectCBName)
		: MeshRenderer(material, terrain->GetMesh(), *terrain->GetSubmesh(), objectCBName)
	{
		SubmeshGeometry* submesh = terrain->GetSubmesh();
		material->SetTexture(mHeightMapName, terrain->GetTexture());

		TerrainObjectConstants* terrainOC = GetObjectData<TerrainObjectConstants*>();

		if (terrainOC != nullptr)
		{
			terrainOC->texHeight = terrain->GetTexture()->Height();
			terrainOC->texWidth = terrain->GetTexture()->Width();
			terrainOC->height = terrain->GetHeightScale();
		}
		
		//SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	}

public:
	const std::string mHeightMapName = "HeightMap";
};
}
