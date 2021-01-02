//***************************************************************************************
// SocoApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/GeometryGenerator.h"
#include "Common/Camera.h"
#include "FrameResource.h"

#include "Soco/Shader.h"
#include "Soco/Material.h"
#include "Soco/Texture.h"

#include "Soco/MeshRenderer.h"
#include "Soco/SkyboxRenderer.h"
#include "Soco/TextureRenderer.h"
#include "Soco/TerrainRenderer.h"

#include "Soco/Util/PrintHelper.h"
#include "Soco/Terrain.h"

#include "Soco/Transform.h"

#include <iostream>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	DirectX::XMFLOAT4 FogColor = { 0.3f, 0.3f, 0.3f, 1.0f };
	float gFogStart = 5.0f;
	float gFogRange = 150.0f;
	DirectX::XMFLOAT2 cbPerObjectPad2;

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light Lights[MaxLights];
};

struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

struct SolarObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
};


struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC;
};


enum class RenderLayer : int
{
	Opaque = 0,
	Skybox,
	AlphaTested,
	Transparent,
	UI,
	Count
};

class SocoApp : public D3DApp
{
public:
    SocoApp(HINSTANCE hInstance);
    SocoApp(const SocoApp& rhs) = delete;
    SocoApp& operator=(const SocoApp& rhs) = delete;
    ~SocoApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    //virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    //virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    //virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
	void OnMouseInput(const GameTimer& gt);

	//void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void LoadTextures();
    void BuildShadersAndInputLayout();
	void BuildMaterials();
	void BuildTerrain();
	void BuildBoxGeometry();
	void BuildSolarGeometry();
    void BuildFrameResources();
	void BuildRenderObjects();
	void BuildTransforms();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<Soco::Renderer*>& objects);

	void GetCommonPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC* input);

	//std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

    UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	// Geometries
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;

	//Textures
	std::unordered_map<std::string, std::unique_ptr<Soco::Texture2D>> mTextures;
	std::unique_ptr<Soco::TextureCube> mCubeMap;
	std::unique_ptr<Soco::RenderTexture> mRenderTexture;

	//Shader & Material
	std::unordered_map<std::string, std::unique_ptr<Soco::Shader>> mShaders;
	std::unordered_map<std::string, std::unique_ptr<Soco::Material>> mMaterials;


	// 按照renderer的种类存储
	std::map<std::string, std::unique_ptr<Soco::MeshRenderer>> mMeshRenderers;
	std::unique_ptr<Soco::SkyboxRenderer> mSkyboxRenderer;
	std::unique_ptr<Soco::TextureRenderer> mTextureRenderer;
	std::unique_ptr<Soco::TerrainRenderer> mTerrainRenderer;

	// 按照render queue存储
	std::vector<Soco::Renderer*> mRenderObjectLayer[(int)RenderLayer::Count];

	Camera mCamera;

    PassConstants mMainPassCB;

//	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
//	XMFLOAT4X4 mView = MathHelper::Identity4x4();
//	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f*XM_PI;
    float mPhi = XM_PIDIV2 - 0.1f;
    float mRadius = 50.0f;

    //POINT mLastMousePos;

	//solar
	const float mEarthRotationRadius = 26;
	Soco::Transform mEarthTransform;
	float mEarthSunTheta = 0;

	const float mMoonEarthRotationRadius = 2.5f;
	Soco::Transform mMoonTransform;

	const float mMercurySunRotationRadius = 8.f;
	Soco::Transform mMercuryTransform;
	float mMercurySunTheta = 0;

	const float mVenusSunRotationRadius = 16.f;
	Soco::Transform mVenusTransform;
	float mVenusSunTheta = 0;

	//terrain
	std::unique_ptr<Soco::Terrain> mTerrain;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{


    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        SocoApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;
		
        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

SocoApp::SocoApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

SocoApp::~SocoApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}



bool SocoApp::Initialize()
{
	mMainWndCaption = L"Soco DX12 App";
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCamera.SetPosition(0.0f, 0.0f, 0.0f);
	mCamera.LookAt(mCamera.GetPosition3f(), { 0, 0, 1 }, { 0, 1, 0 });

	LoadTextures();
    BuildShadersAndInputLayout();
	BuildMaterials();
	BuildTerrain();
	BuildBoxGeometry();
	BuildSolarGeometry();
    BuildRenderObjects();
	BuildTransforms();
    BuildFrameResources();


    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void SocoApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    //XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    //XMStoreFloat4x4(&mProj, P);
	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 2000.0f);

	if (mRenderTexture != nullptr)
	{
		D3D12_RESOURCE_DESC backbufferDesc = mSwapChainBuffer[0]->GetDesc();
		mRenderTexture->Resize(backbufferDesc.Width, backbufferDesc.Height);
	}
	
}

void SocoApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
	OnMouseInput(gt);
	//UpdateCamera(gt);
	mCamera.UpdateViewMatrix();

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

	//Earth
	Soco::MeshRenderer* earthMeshRenderer = mMeshRenderers["Earth"].get();
	SolarObjectConstants soc;
	mEarthSunTheta += gt.DeltaTime() * 0.5f;//公转
	float x = mEarthRotationRadius * cos(mEarthSunTheta);
	float z = mEarthRotationRadius * sin(mEarthSunTheta);

	mEarthTransform.SetGlobalPosition({ x, 0, z });
	mEarthTransform.Rotate({ 0, gt.DeltaTime() * 2 , 0 });
	XMStoreFloat4x4(&soc.World, XMMatrixTranspose(mEarthTransform.GetGlobalMatrixM()));
	earthMeshRenderer->SetObjectData(&soc);

	//Moon
	Soco::MeshRenderer* moonMeshRenderer = mMeshRenderers["Moon"].get();
	XMStoreFloat4x4(&soc.World, XMMatrixTranspose(mMoonTransform.GetGlobalMatrixM()));
	moonMeshRenderer->SetObjectData(&soc);

	//Mercury
	Soco::MeshRenderer* mercuryMeshRenderer = mMeshRenderers["Mercury"].get();
	mMercurySunTheta += gt.DeltaTime() * 0.3f;
	x = mMercurySunRotationRadius * cos(mMercurySunTheta);
	z = mMercurySunRotationRadius * sin(mMercurySunTheta);
	mMercuryTransform.SetGlobalPosition({ x, 0, z });
	XMStoreFloat4x4(&soc.World, XMMatrixTranspose(mMercuryTransform.GetGlobalMatrixM()));
	mercuryMeshRenderer->SetObjectData(&soc);

	//Venus
	Soco::MeshRenderer* venusMeshRenderer = mMeshRenderers["Venus"].get();
	mVenusSunTheta += gt.DeltaTime() * 0.4f;
	x = mVenusSunRotationRadius * cos(mVenusSunTheta);
	z = mVenusSunRotationRadius * sin(mVenusSunTheta);
	mVenusTransform.SetGlobalPosition({ x, 0, z });
	XMStoreFloat4x4(&soc.World, XMMatrixTranspose(mVenusTransform.GetGlobalMatrixM()));
	venusMeshRenderer->SetObjectData(&soc);


	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
}

void SocoApp::Draw(const GameTimer& gt)
{

    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    //ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), nullptr));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferRTV(), (float*)&mMainPassCB.FogColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferRTV(), true, &DepthStencilView());

	ID3D12DescriptorHeap* srvDescriptorHeap = mCbvSrvUavHeap->GetDescriptorHeap();
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

 //   DrawRenderItems(mCommandList.Get(), mRenderObjectLayer[(int)RenderLayer::Opaque]);

	//DrawRenderItems(mCommandList.Get(), mRenderObjectLayer[(int)RenderLayer::AlphaTested]);
	//
	//DrawRenderItems(mCommandList.Get(), mRenderObjectLayer[(int)RenderLayer::Skybox]);

	//DrawRenderItems(mCommandList.Get(), mRenderObjectLayer[(int)RenderLayer::Transparent]);

	//DrawRenderItems(mCommandList.Get(), mRenderObjectLayer[(int)RenderLayer::UI]);

	for (std::vector<Soco::Renderer*>& RenderQueue : mRenderObjectLayer)
	{
		DrawRenderItems(mCommandList.Get(), RenderQueue);
	}

	//Compute Shader
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	mRenderTexture->TransitionTo(mCommandList.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	mShaders["ShadeRed"]->SetComputePipelineState(mCommandList.Get());
	mShaders["ShadeRed"]->SetComputeRootSignature(mCommandList.Get());
	mShaders["ShadeRed"]->SetTexture(mCommandList.Get(), "gInput", CurrentBackBufferSRV());
	mShaders["ShadeRed"]->SetTexture(mCommandList.Get(), "gOutput", mRenderTexture->UAV());

	mCommandList->Dispatch(ceil(mClientWidth / 32.0f), ceil(mClientHeight / 32.0f), 1);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	mRenderTexture->TransitionTo(mCommandList.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);

	mCommandList->CopyResource(CurrentBackBuffer(), mRenderTexture->GetResource());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));


    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;


    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void SocoApp::OnKeyboardInput(const GameTimer& gt)
{
	//auto kb = mKeyboard->GetState();
	static bool TerrainWireframe = false;
	if (GetKeyDown(Key::D1))
	{
		if (!TerrainWireframe)
		{
			D3D12_RASTERIZER_DESC rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
			rasterizer.CullMode = D3D12_CULL_MODE_NONE;
			mMaterials["Terrain"]->SetRasterizerState(rasterizer);
			TerrainWireframe = true;
		}
		else
		{
			D3D12_RASTERIZER_DESC rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			mMaterials["Terrain"]->SetRasterizerState(rasterizer);
			TerrainWireframe = false;
		}

	}

	if (GetKeyDown(Key::C))
	{
		rdoc_api->TriggerCapture();
		if(!rdoc_api->IsTargetControlConnected())
			rdoc_api->LaunchReplayUI(1, nullptr);
	}

	const float dt = gt.DeltaTime();
	const float speed = 50.0f;
	float moveSpeed = GetKey(Key::LeftShift) ? 2.5f * speed : speed;

	if (GetKey(Key::W))
		mCamera.Walk(moveSpeed * dt);
	if (GetKey(Key::S))
		mCamera.Walk(-moveSpeed * dt);
	if (GetKey(Key::A))
		mCamera.Strafe(-moveSpeed * dt);
	if (GetKey(Key::D))
		mCamera.Strafe(moveSpeed * dt);

}
 
void SocoApp::OnMouseInput(const GameTimer& gt)
{
	POINT moveDelta = GetMouseMove();
	if (GetKey(MouseKey::LEFT_BUTTON))
	{
		mCamera.Pitch(XMConvertToRadians(0.25f*static_cast<float>(moveDelta.y)));
		mCamera.RotateY(XMConvertToRadians(0.25f*static_cast<float>(moveDelta.x)));
	}
}

void SocoApp::UpdateObjectCBs(const GameTimer& gt)
{
	for (std::vector<Soco::Renderer*>& renderLayer : mRenderObjectLayer)
	{
		for (Soco::Renderer* renderer : renderLayer)
		{
			renderer->Update(mCurrFrameResourceIndex);
		}
	}
}

void SocoApp::UpdateMaterialCBs(const GameTimer& gt)
{
	for (auto& [name, material] : mMaterials)
	{
		material->Update(mCurrFrameResourceIndex);
	}
}

void SocoApp::UpdateMainPassCB(const GameTimer& gt)
{
	//XMMATRIX view = XMLoadFloat4x4(&mView);
	//XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	static bool updateEyePos = true;

	if (GetKey(Key::H))
		updateEyePos = !updateEyePos;

	if(updateEyePos)
		mMainPassCB.EyePosW = mCamera.GetPosition3f();

	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.8f };

	mMainPassCB.Lights[1].Position = { 0 ,0, 0 };
	mMainPassCB.Lights[1].Strength = { 1 ,1, 1 };

	//mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	//mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	//mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	//mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void SocoApp::LoadTextures()
{
	mTextures["fenceTex"] = std::make_unique<Soco::Texture2D>("fenceTex", L"../Textures/WireFence.dds");

	mTextures["EarthDay"] = std::make_unique<Soco::Texture2D>("EarthDay", L"../Textures/Resources/1/EarthDay.dds");
	mTextures["EarthNight"] = std::make_unique<Soco::Texture2D>("EarthNight", L"../Textures/Resources/1/EarthNight.dds");
	mTextures["EarthCloud"] = std::make_unique<Soco::Texture2D>("EarthCloud", L"../Textures/Resources/1/EarthClouds.dds");
	mTextures["Sun"] = std::make_unique<Soco::Texture2D>("Sun", L"../Textures/Resources/1/NL5.dds");
	mTextures["Moon"] = std::make_unique<Soco::Texture2D>("Moon", L"../Textures/Resources/1/moon.dds");
	mTextures["Mercury"] = std::make_unique<Soco::Texture2D>("Mercury", L"../Textures/Resources/1/Mercury.dds");
	mTextures["Venus"] = std::make_unique<Soco::Texture2D>("Venus", L"../Textures/Resources/1/Venus.dds");

	mCubeMap = std::make_unique<Soco::TextureCube>("GrassCubeMap", L"../Textures/grasscube1024.dds");

	Soco::RenderTextureFormat rtFormat;
	rtFormat.ResourceFormat = mBackBufferFormat;
	rtFormat.SrvFormat = mBackBufferFormat;
	rtFormat.UavFormat = mBackBufferFormat;

	D3D12_RESOURCE_DESC backbufferDesc = mSwapChainBuffer[0]->GetDesc();
	mRenderTexture = std::make_unique<Soco::RenderTexture>(backbufferDesc.Width, backbufferDesc.Height, rtFormat);
}


void SocoApp::BuildShadersAndInputLayout()
{

	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	Soco::ShaderStage ss;
	ss.vs = "VS";
	ss.ps = "PS";

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mShaders["opaque"] = std::make_unique<Soco::Shader>(L"Shaders\\Default.hlsl", defines, ss);
	mShaders["alphaTested"] = std::make_unique<Soco::Shader>(L"Shaders\\Default.hlsl", alphaTestDefines, ss);
	mShaders["Earth"] = std::make_unique<Soco::Shader>(L"Shaders\\Earth.hlsl", nullptr, ss);
	mShaders["Sun"] = std::make_unique<Soco::Shader>(L"Shaders\\Sun.hlsl", nullptr, ss);
	//金星、水星、月亮 共用一个shader
	mShaders["Moon"] = std::make_unique<Soco::Shader>(L"Shaders\\Moon.hlsl", nullptr, ss);

	//Skybox
	mShaders["Skybox"] = std::make_unique<Soco::Shader>(L"Shaders\\Skybox.hlsl", nullptr, ss);

	//Terrain
	std::vector<D3D12_INPUT_ELEMENT_DESC> terrainInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	Soco::ShaderStage TessShaderStage;
	TessShaderStage.vs = "VS";
	TessShaderStage.ps = "PS";
	TessShaderStage.hs = "HS";
	TessShaderStage.ds = "DS";

	//mShaders["Terrain"] = std::make_unique<Soco::Shader>(L"Shaders\\Terrain.hlsl", nullptr, ss, &terrainInputLayout);
	mShaders["TerrainTess"] = std::make_unique<Soco::Shader>(L"Shaders\\TerrainTess.hlsl", nullptr, TessShaderStage, &terrainInputLayout);

	Soco::ShaderStage ComputeStage;
	ComputeStage.cs = "ShadeRed";

	mShaders["ShadeRed"] = std::make_unique<Soco::Shader>(L"Shaders\\ShadeRed.hlsl", nullptr, ComputeStage);
}

void SocoApp::BuildMaterials()
{

	Soco::MaterialState ms;
	ms.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	ms.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	ms.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	GetCommonPSODesc(&psoDesc);
	MaterialConstants mc;
	mc.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mc.FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	mc.Roughness = 0.125f;


	//land
	//mMaterials["grass"] = std::make_unique<Soco::Material>(mShaders["opaque"].get(), &ms, &psoDesc, "cbMaterial");
	//mMaterials["grass"]->SetTexture("gDiffuseMap", mTextures["grassTex"].get());

	//mMaterials["grass"]->SetMaterialData(&mc);

	//water
	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	ms.blendState.RenderTarget[0] = transparencyBlendDesc;

	//mMaterials["water"] = std::make_unique<Soco::Material>(mShaders["opaque"].get(), &ms, &psoDesc, "cbMaterial");
	//mMaterials["water"]->SetTexture("gDiffuseMap", mTextures["waterTex"].get());

	//mc.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	//mc.FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//mc.Roughness = 0.0f;
	//mMaterials["water"]->SetMaterialData(&mc);

	//wirefence
	ms.rasterizeState.CullMode = D3D12_CULL_MODE_NONE;
	mMaterials["wirefence"] = std::make_unique<Soco::Material>(mShaders["alphaTested"].get(), &ms, nullptr, "cbMaterial");
	mMaterials["wirefence"]->SetTexture("gDiffuseMap", mTextures["fenceTex"].get());

	mc.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mc.FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	mc.Roughness = 0.125f;

	mMaterials["wirefence"]->SetMaterialData(&mc);


	//earth
	Soco::MaterialState earthMS;
	earthMS.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	earthMS.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	earthMS.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	mMaterials["Earth"] = std::make_unique<Soco::Material>(mShaders["Earth"].get(), &earthMS, nullptr, "cbMaterial");
	mMaterials["Earth"]->SetTexture("EarthDayMap", mTextures["EarthDay"].get());
	mMaterials["Earth"]->SetTexture("EarthNightMap", mTextures["EarthNight"].get());
	mMaterials["Earth"]->SetTexture("EarthCloudMap", mTextures["EarthCloud"].get());

	//sun
	Soco::MaterialState sunMS;
	sunMS.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	sunMS.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//sunMS.rasterizeState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//sunMS.rasterizeState.CullMode = D3D12_CULL_MODE_NONE;
	sunMS.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	mMaterials["Sun"] = std::make_unique<Soco::Material>(mShaders["Sun"].get(), &sunMS);
	mMaterials["Sun"]->SetTexture("SunNoiseMap", mTextures["Sun"].get());

	//Moon
	Soco::MaterialState moonMS;
	moonMS.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	moonMS.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	moonMS.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	mMaterials["Moon"] = std::make_unique<Soco::Material>(mShaders["Moon"].get());
	mMaterials["Moon"]->SetTexture("MoonMap", mTextures["Moon"].get());

	mMaterials["Mercury"] = std::make_unique<Soco::Material>(mShaders["Moon"].get());
	mMaterials["Mercury"]->SetTexture("MoonMap", mTextures["Mercury"].get());

	mMaterials["Venus"] = std::make_unique<Soco::Material>(mShaders["Moon"].get());
	mMaterials["Venus"]->SetTexture("MoonMap", mTextures["Venus"].get());

	//skybox
	Soco::MaterialState skyboxMS;
	skyboxMS.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	skyboxMS.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	skyboxMS.rasterizeState.CullMode = D3D12_CULL_MODE_NONE;
	skyboxMS.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	skyboxMS.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	mMaterials["Skybox"] = std::make_unique<Soco::Material>(mShaders["Skybox"].get(), &skyboxMS, nullptr, "");
	mMaterials["Skybox"]->SetTexture("gCubeMap", mCubeMap.get());

	//terrain
	Soco::MaterialState terrainMS;
	terrainMS.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	terrainMS.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	terrainMS.rasterizeState.CullMode = D3D12_CULL_MODE_NONE;
	//terrainMS.rasterizeState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	terrainMS.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//mMaterials["Terrain"] = std::make_unique<Soco::Material>(mShaders["Terrain"].get());
	mMaterials["Terrain"] = std::make_unique<Soco::Material>(mShaders["TerrainTess"].get());
}

void SocoApp::BuildBoxGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);

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

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->VertexBufferGPU->SetName(L"Vertex Buffer");
	geo->VertexBufferUploader->SetName(L"Vertex Buffer Uploader");

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
	geo->IndexBufferGPU->SetName(L"Index Buffer");
	geo->IndexBufferUploader->SetName(L"Index Buffer Uploader");

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["box"] = submesh;

	mGeometries["boxGeo"] = std::move(geo);
}

void SocoApp::BuildSolarGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1, 20, 20);

	std::vector<Vertex> vertices(sphere.Vertices.size());
	for (size_t i = 0; i < sphere.Vertices.size(); ++i)
	{
		vertices[i].Pos = sphere.Vertices[i].Position;
		vertices[i].Normal = sphere.Vertices[i].Normal;
		vertices[i].TexC = sphere.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	
	std::vector<std::uint16_t> indices = sphere.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "solarGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->VertexBufferGPU->SetName(L"Vertex Buffer");
	geo->VertexBufferUploader->SetName(L"Vertex Buffer Uploader");

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
	geo->IndexBufferGPU->SetName(L"Index Buffer");
	geo->IndexBufferUploader->SetName(L"Index Buffer Uploader");

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["sphere"] = submesh;

	mGeometries["solar"] = std::move(geo);
}

void SocoApp::GetCommonPSODesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC* input)
{
	ZeroMemory(input, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	input->SampleMask = UINT_MAX;
	input->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	input->NumRenderTargets = 1;
	input->RTVFormats[0] = mBackBufferFormat;
	input->SampleDesc.Count = m4xMsaaState ? 4 : 1;
	input->SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	input->DSVFormat = mDepthStencilFormat;
}

void SocoApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), sizeof(PassConstants)));
}


void SocoApp::BuildRenderObjects()
{

	const char* OBJECT_CB_NAME = "cbPerObject";

	//box
	MeshGeometry* boxMesh = mGeometries["boxGeo"].get();
	auto boxRitem = std::make_unique<Soco::MeshRenderer>(mMaterials["wirefence"].get(), boxMesh, boxMesh->DrawArgs["box"], OBJECT_CB_NAME);


	ObjectConstants* boxOC = boxRitem->GetObjectData<ObjectConstants*>();
	boxOC->TexTransform = MathHelper::Identity4x4();
	boxOC->World = MathHelper::Identity4x4();

	//mRenderObjectLayer[(int)RenderLayer::AlphaTested].push_back(boxRitem.get());
	//mAllObject.push_back(std::move(boxRitem));

	//Earth
	MeshGeometry* solarMesh = mGeometries["solar"].get();
	auto earthRitem = std::make_unique<Soco::MeshRenderer>(mMaterials["Earth"].get(), solarMesh, solarMesh->DrawArgs["sphere"], OBJECT_CB_NAME);

	SolarObjectConstants* solarOC = earthRitem->GetObjectData<SolarObjectConstants*>();
	XMStoreFloat4x4(&solarOC->World, XMMatrixTranspose(XMMatrixScaling(1, 1, 1)*XMMatrixTranslation(20, 0, 20)));
	mRenderObjectLayer[(int)RenderLayer::Opaque].push_back(earthRitem.get());
	mMeshRenderers["Earth"] = std::move(earthRitem);

	//Sun
	auto sunRitem = std::make_unique<Soco::MeshRenderer>(mMaterials["Sun"].get(), solarMesh, solarMesh->DrawArgs["sphere"], OBJECT_CB_NAME);
	solarOC = sunRitem->GetObjectData<SolarObjectConstants*>();
	XMStoreFloat4x4(&solarOC->World, XMMatrixTranspose(XMMatrixScaling(6, 6, 6)));
	mRenderObjectLayer[(int)RenderLayer::Opaque].push_back(sunRitem.get());
	mMeshRenderers["Sun"] = std::move(sunRitem);

	//Moon
	auto moonRitem = std::make_unique<Soco::MeshRenderer>(mMaterials["Moon"].get(), solarMesh, solarMesh->DrawArgs["sphere"], OBJECT_CB_NAME);
	mRenderObjectLayer[(int)RenderLayer::Opaque].push_back(moonRitem.get());
	mMeshRenderers["Moon"] = std::move(moonRitem);

	//Mercury
	auto MercuryRitem = std::make_unique<Soco::MeshRenderer>(mMaterials["Mercury"].get(), solarMesh, solarMesh->DrawArgs["sphere"], OBJECT_CB_NAME);
	mRenderObjectLayer[(int)RenderLayer::Opaque].push_back(MercuryRitem.get());
	mMeshRenderers["Mercury"] = std::move(MercuryRitem);

	//Venus
	auto VenusRitem = std::make_unique<Soco::MeshRenderer>(mMaterials["Venus"].get(), solarMesh, solarMesh->DrawArgs["sphere"], OBJECT_CB_NAME);
	mRenderObjectLayer[(int)RenderLayer::Opaque].push_back(VenusRitem.get());
	mMeshRenderers["Venus"] = std::move(VenusRitem);

	//Skybox
	auto SkyboxRitem = std::make_unique<Soco::SkyboxRenderer>(mMaterials["Skybox"].get());
	mRenderObjectLayer[(int)RenderLayer::Skybox].push_back(SkyboxRitem.get());
	mSkyboxRenderer = std::move(SkyboxRitem);

	//Texture2D
	auto TextureRitem = std::make_unique<Soco::TextureRenderer>(mTerrain->GetTexture());
	Soco::TextureRenderer::TexPositionCB& texCB = TextureRitem->GetTexPosCB();
	texCB.size = { 0.2f, 0.2f };
	mRenderObjectLayer[(int)RenderLayer::UI].push_back(TextureRitem.get());
	mTextureRenderer = std::move(TextureRitem);
}

void SocoApp::BuildTransforms()
{
	mEarthTransform = Soco::Transform({ mEarthRotationRadius, 0, 0 }, { 0, 0, 0 }, { 2, 2, 2 });
	mMoonTransform = Soco::Transform({ mMoonEarthRotationRadius, 0, 0 }, { 0,0,0 }, { 0.5f, 0.5f, 0.5f }, &mEarthTransform);
	mMercuryTransform = Soco::Transform({ mMercurySunRotationRadius, 0, 0 }, { 0, 0, 0 }, { 1.5f, 1.5f, 1.5f });
	mVenusTransform = Soco::Transform({ mVenusSunRotationRadius, 0, 0 }, { 0, 0, 0 }, { 1.7f, 1.7f, 1.7f });
}

void SocoApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<Soco::Renderer*>& objects)
{

	auto passCB = mCurrFrameResource->PassCB->Resource();

    // For each render item...
    for(size_t i = 0; i < objects.size(); ++i)
    {
        auto object = objects[i];
		object->SetPipelineState(cmdList);
		object->SetGraphicsRootSignature(cmdList);

		object->Setup(cmdList, mCurrFrameResourceIndex);

		object->SetCBV(cmdList, "cbPass", passCB->GetGPUVirtualAddress());

		//std::cout << "Draw Call IndexCount" << std::endl;
		object->DrawIndexedInstanced(cmdList);
    }
}

void SocoApp::BuildTerrain()
{
	const char* OBJECT_CB_NAME = "cbPerObject";
	mTerrain = std::make_unique<Soco::Terrain>("../Textures/HeightMaps/heightmap2.png");

	auto TerrainRitem = std::make_unique<Soco::TerrainRenderer>(mTerrain.get(), mMaterials["Terrain"].get(), OBJECT_CB_NAME);
	mRenderObjectLayer[(int)RenderLayer::Opaque].push_back(TerrainRitem.get());
	mTerrainRenderer = std::move(TerrainRitem);

}