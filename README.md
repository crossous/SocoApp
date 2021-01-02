# SocoApp
Crossous's DX12 FrameWork

Crossous的DirectX12 框架，对DX12做了一些封装
# 说明
当前处于开发参考阶段，部分本人没遇到的功能还未具有通用性

# 引用

C++ 17及以上

**magic_enum** 用于显示错误时的枚举值。

**D3D12MemAlloc** 用于简化显存分配，无需考虑显存是Placed还是Committed Resource。

**DirectX Tool Kit For DirectX 12** 简化DDS纹理读取、键盘鼠标输入

**lodepng** 读取PNG图片文件

**renderdoc_app** 内置可开启renderdoc截取分析

基于**Introduction to 3D Game Programming with DirectX 12 **的框架编写

# 封装

### Shader

&emsp;&emsp;DX12的HLSL Shader关联到DX12的顶点布局（Input Layout）、寄存器布局（Root Signature），在《Intriduction……DX12》(后称为龙书)中，作者用了一个Uber Shader完成大部分功能，全局几乎只用了一个寄存器布局，实际编写程序中，不同Shader拥有不同的布局。

&emsp;&emsp;并且龙书中，寄存器布局、视图在堆中的位置，是用大脑来记录，属于硬编码在程序中，而此框架的Shader封装尽量让Shader自己记录寄存器布局。

&emsp;&emsp;本人用了DX12反射功能D3D12Reflection对HLSL信息进行读取，来初始化Shader的Root Signature，并记录布局。

&emsp;&emsp;Input Layout信息也可以通过反射读取，包括类型、Semantic Name等，但HLSL的Input Layout通常只表示自己需要什么数据，而步长等其他数据，还要由网格数据决定，因此Shader签名暂时留下默认Input Layout读取方式，也可以在构造函数中，由外界传入。

&emsp;&emsp;使用方法案例：

```c++
// construct:
const D3D_SHADER_MACRO defines[] = // defines // options
{
    "FOG", "1",
    NULL, NULL
};

Soco::ShaderStage ss;//entry point
ss.vs = "VS";
ss.ps = "PS";

mShaders["opaque"] = std::make_unique<Soco::Shader>(L"Shaders\\Default.hlsl", defines, ss);
```

&emsp;&emsp;Shader还能帮你填写PSO描述符所需要的信息，以及设置常量缓冲区、纹理，这些通常不需要你自己来做，而是由我的Material封装来处理，但接口依旧保留。

&emsp;&emsp;用于初始化PSO的几个方法：

```c++
//Initialize Graphics PSO Desc
void SetPSODescShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);//将hlsl二进制程序填写到pso中
void SetPSODescTopology(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);//填写拓扑类型 point\line\triangle\patch
void SetPSODescRootSignature(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);//填写Root Signature
void SetPSODescInputLayout(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);//填写InputLayout
```

&emsp;&emsp;用于SetPass的几个方法：

```C++
//设置根签名到当前CommandList
void SetGraphicsRootSignature(ID3D12GraphicsCommandList* cmdList);
//设置图元类型
void SetIASetPrimitiveTopology(ID3D12GraphicsCommandList* cmdList);
```

&emsp;&emsp;设置常量缓冲区、纹理的方法：

```c++
void SetConstantBufferView(ID3D12GraphicsCommandList* cmdList, const std::string& variableName, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
void SetTexture(ID3D12GraphicsCommandList* cmdList, const std::string& textureName, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
```

&emsp;&emsp;支持Compute Shader

```c++
void SetComputeRootSignature(ID3D12GraphicsCommandList* cmdList)
void SetComputePipelineState(ID3D12GraphicsCommandList* cmdList)
```

&emsp;&emsp;支持曲面细分，当你传入的ShaderStage包括DomainShader和HullShader的EntryPoint时，会将你的图元拓扑类型改成相应枚举值。

### Texture

&emsp;&emsp;和Unity一样，Texture只是一个抽象基类，指定子类必须实现能提供创建着色器资源视图的方法GetSRV，基于这个基类，我实现了Texture2D、TextureCube、RenderTexture。

&emsp;&emsp;和前两个不同，RenderTexture不但可以作为着色器资源，还可以作为渲染目标(RTV)、深度模版缓冲(DSV)，并且有时能支持无序写入、访问(UAV)。

&emsp;&emsp;因此RenderTexture构建时需要传入一个格式：

```c++
struct RenderTextureFormat
{
	DXGI_FORMAT ResourceFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT SrvFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT UavFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT RtvFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT DsvFormat = DXGI_FORMAT_UNKNOWN;
};
```

&emsp;&emsp;其中ResourceFormat是纹理资源格式，后面4个格式是分别作为其他用途时的格式；例如龙书的ShadowMap章节中，ShadowMap纹理资源格式本身被指定为**DXGI_FORMAT_R24G8_TYPELESS**，作为着色器资源时，格式为**DXGI_FORMAT_R24_UNORM_X8_TYPELESS**，作为深度模版缓冲时，为：**DXGI_FORMAT_D24_UNORM_S8_UINT**；并非是所有格式都可以同时绑定，根据[Microsoft文档](https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_flags)，RT和DS、UA和DS的视图不能同时被创建。

&emsp;&emsp;使用方法：

```c++
//Texture2D
mTextures["EarthDay"] = std::make_unique<Soco::Texture2D>("EarthDay", 			L"../Textures/Resources/1/EarthDay.dds");
mTextures["EarthNight"] = std::make_unique<Soco::Texture2D>("EarthNight", 		L"../Textures/Resources/1/EarthNight.dds");
mTextures["EarthCloud"] = std::make_unique<Soco::Texture2D>("EarthCloud", 		L"../Textures/Resources/1/EarthClouds.dds");

//TextureCube
mCubeMap = std::make_unique<Soco::TextureCube>("GrassCubeMap", 					L"../Textures/grasscube1024.dds");

//RenderTexture
Soco::RenderTextureFormat rtFormat;
rtFormat.ResourceFormat = mBackBufferFormat;
rtFormat.SrvFormat = mBackBufferFormat;
rtFormat.UavFormat = mBackBufferFormat;

D3D12_RESOURCE_DESC backbufferDesc = mSwapChainBuffer[0]->GetDesc();
mRenderTexture = std::make_unique<Soco::RenderTexture>(backbufferDesc.Width, backbufferDesc.Height, rtFormat);
```



### Material

&emsp;&emsp;Material需要传入使用的Shader、[光栅化、深度模版缓冲状态、默认PSO描述符、Material数据常量缓冲区名称]：

&emsp;&emsp;`Material(Shader* shader, MaterialState* drawState = nullptr, D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc = nullptr, const std::string& matCBName = "");`

&emsp;&emsp;初始化事例：

```c++
Soco::MaterialState earthMS;
earthMS.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
earthMS.rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
earthMS.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

mMaterials["Earth"] = std::make_unique<Soco::Material>(mShaders["Earth"].get(), 		&earthMS, nullptr, "cbMaterial");
mMaterials["Earth"]->SetTexture("EarthDayMap", mTextures["EarthDay"].get());
mMaterials["Earth"]->SetTexture("EarthNightMap", mTextures["EarthNight"].get());
mMaterials["Earth"]->SetTexture("EarthCloudMap", mTextures["EarthCloud"].get());
```

&emsp;&emsp;Material可以帮助管理常量缓冲区数据（当前常量缓冲区使用方式并不好，申请、使用方式过于零散，与SRP Batch的方法背道而驰，还需要整改，但当前拥有脏数据功能，不修改数据的情况下不会上传数据给GPU）。

&emsp;&emsp;当前可以设置和获取MaterialObject：

```C++
SetMaterialData
GetMaterialData
```

&emsp;&emsp;之所以未放函数签名，因为使用了模版元的SFINEA，能察觉到使用的是引用还是指针，或者直接值传递。

&emsp;&emsp;由于上述的设计问题，Material暂时暴露出Update接口，当引擎Update时，需要调用所有Material的Update方法，用来更新常量缓冲区数据：

```c++
for (auto& [name, material] : mMaterials)
{
	material->Update(mCurrFrameResourceIndex);
}
```

&emsp;&emsp;Material还支持设置PSO和SetPass功能：

```c++
//设置PSO
void SetPipelineState(ID3D12GraphicsCommandList* cmdList);
//设置常量缓冲区到对应slot中
void Setup(ID3D12GraphicsCommandList* cmdList, int currentFrame)
```

&emsp;&emsp;和前面部分Shader接口一样，一般不直接调用，而是交给我封装的Renderer来调用。

### Renderer

&emsp;&emsp;和Unity一样，Renderer只是一个抽象基类，管理常量缓冲区数据和更新，SetPass、DrawCall等。

&emsp;&emsp;基于Renderer实现了MeshRenderer、SkyboxRenderer、TextureRenderer、TerrainRenderer。

### 输入模块

&emsp;&emsp;基于XTK实现的输入模块，提供了以下几个函数：

```c++
bool D3DApp::GetKey(Key);
bool D3DApp::GetKeyDown(Key);
bool D3DApp::GetKeyUp(Key);
bool D3DApp::GetKey(MouseKey);
bool D3DApp::GetKeyDown(MouseKey);
bool D3DApp::GetKeyUp(MouseKey);
POINT D3DApp::GetMouseMove();
POINT D3DApp::GetMousePosition();
```

&emsp;&emsp;使用方式：

```C++
if (GetKeyDown(Key::C))
{
	rdoc_api->TriggerCapture();
	if(!rdoc_api->IsTargetControlConnected())
		rdoc_api->LaunchReplayUI(1, nullptr);
}
```





