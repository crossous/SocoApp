#include "Shader.h"
#include <exception>

//#include "../Common/magic_enum.hpp"
#include "Util/Redefine.h"
#include "Util/RootSignatureManager.h"

using Microsoft::WRL::ComPtr;

namespace Soco{

//Shader工具类，当申请描述符表时需要一个CD3DX12_DESCRIPTOR_RANGE对象
//但传入根签名时是传入地址，因此不能重复使用一个RANGE对象
//所以池化，用时获取，用完释放
//之所以不直接用vector，是因为当触发扩容时，vector会重新分配地址空间，导致注册根参数时的数据发生变化
class DescriptorRangePool
{
public:
	static DescriptorRangePool* GetInstance()
	{
		static DescriptorRangePool* instance = new DescriptorRangePool();
		return instance;
	}

	CD3DX12_DESCRIPTOR_RANGE* GetDescriptorRange(UINT num)
	{
		if (allocated + num <= mRanges.size())
		{
			CD3DX12_DESCRIPTOR_RANGE* res = &mRanges[allocated];
			allocated += num;
			return res;
		}
		else {
			CD3DX12_DESCRIPTOR_RANGE* tempRange = new CD3DX12_DESCRIPTOR_RANGE[num];
			mTempRanges.push_back({tempRange, num});
			return tempRange;
		}
	}

	void Release()
	{
		if (size_t IncreaseSize = 0; mTempRanges.size() != 0)
		{
			for (auto [tempRange, size] : mTempRanges)
			{
				IncreaseSize += size;
				delete[] tempRange;
			}
			
			mTempRanges.swap(std::vector<std::pair<CD3DX12_DESCRIPTOR_RANGE*, size_t>>());
			mRanges.resize(allocated + IncreaseSize);
		}
		allocated = 0;
	}

private:
	DescriptorRangePool(){};

	std::vector<CD3DX12_DESCRIPTOR_RANGE> mRanges;
	std::vector<std::pair<CD3DX12_DESCRIPTOR_RANGE*, size_t>> mTempRanges;
	int allocated = 0;

};

//Shader::Shader(ID3D12Device* device, const std::wstring& filename, const D3D_SHADER_MACRO* defines, ShaderStage stage) {
Shader::Shader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, ShaderStage stage, std::vector<D3D12_INPUT_ELEMENT_DESC>* inputLayout) {
	mName = filename;

	//stage = (ShaderStage::VS | ShaderStage::PS);
	if (stage.cs == nullptr && (stage.vs == nullptr || stage.ps == nullptr)) {
		throw std::exception("非Compute Shader的VS或PS阶段为必选项");
	}

	if (stage.cs == nullptr)
	{
		VS = d3dUtil::CompileShader(filename, defines, stage.vs, "vs_5_1");
		PS = d3dUtil::CompileShader(filename, defines, stage.ps, "ps_5_1");

		if (stage.ds != nullptr && stage.hs != nullptr)
		{
			DS = d3dUtil::CompileShader(filename, defines, stage.ds, "ds_5_1");
			HS = d3dUtil::CompileShader(filename, defines, stage.hs, "hs_5_1");
		}

		if (stage.gs != nullptr)
		{
			GS = d3dUtil::CompileShader(filename, defines, stage.gs, "gs_5_1");
		}
	}
	else
	{
		CS = d3dUtil::CompileShader(filename, defines, stage.cs, "cs_5_1");
	}
	BuildRootSignature();
	if (inputLayout != nullptr)
		mInputLayout = *inputLayout;
	else if(IsGraphicsShader())
		BuildInputLayout(VS);

	if (IsComputeShader())
		BuildComputePipelineState();
}

void Shader::BuildRootSignature()
{
	InitInput(VS, D3D12_SHADER_VISIBILITY_VERTEX);
	InitInput(PS, D3D12_SHADER_VISIBILITY_PIXEL);
	InitInput(DS, D3D12_SHADER_VISIBILITY_DOMAIN);
	InitInput(HS, D3D12_SHADER_VISIBILITY_HULL);
	InitInput(GS, D3D12_SHADER_VISIBILITY_GEOMETRY);
	InitInput(CS, D3D12_SHADER_VISIBILITY_ALL);

	std::vector<CD3DX12_ROOT_PARAMETER> slotRootParameter;
	slotRootParameter.reserve(mVariables.size());

	for (auto ite = mVariables.begin(); ite != mVariables.end(); ++ite) {
		ShaderVariable& variable = ite->second;

		if (variable.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER) {
			slotRootParameter.emplace_back();
			slotRootParameter.back().InitAsConstantBufferView(variable.BindPoint, variable.Space);
			variable.rootSlot = slotRootParameter.size() - 1;
		}
		else if (variable.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE) {

			CD3DX12_DESCRIPTOR_RANGE* texTable = DescriptorRangePool::GetInstance()->GetDescriptorRange(1);

			texTable->Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, variable.BindCount, variable.BindPoint, variable.Space);

			slotRootParameter.emplace_back();
			slotRootParameter.back().InitAsDescriptorTable(1, texTable, variable.Visiblity);

			variable.rootSlot = slotRootParameter.size() - 1;
			mTextureSlot[variable.Name] = variable.rootSlot;
			
		}
		else if (variable.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER) {
			continue;
		}
		else if (variable.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWTYPED)
		{
			CD3DX12_DESCRIPTOR_RANGE* texTable = DescriptorRangePool::GetInstance()->GetDescriptorRange(1);
			texTable->Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, variable.BindCount, variable.BindPoint, variable.Space);
			slotRootParameter.emplace_back();
			slotRootParameter.back().InitAsDescriptorTable(1, texTable, variable.Visiblity);

			variable.rootSlot = slotRootParameter.size() - 1;

		}
		else {
			std::string res = "此shader variable类型未做处理，请检查并添加.\n";
			res += "名称为: ";
			res += variable.Name;
			res += " 值为: ";
			res += std::to_string(variable.Type);
			res += " 类型为: ";
			res += magic_enum::enum_name(static_cast<Redefine::D3D_SHADER_INPUT_TYPE>(variable.Type));
			std::cout << res << std::endl;
			throw std::exception(res.c_str());
		}
	}



	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> staticSamplers = d3dUtil::GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(slotRootParameter.size(), slotRootParameter.data(),
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		std::cout << (char*)errorBlob->GetBufferPointer() << std::endl;
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	//ThrowIfFailed(device->CreateRootSignature(
	//	0,
	//	serializedRootSig->GetBufferPointer(),
	//	serializedRootSig->GetBufferSize(),
	//	IID_PPV_ARGS(mRootSignature.GetAddressOf())
	//));
	mRootSignature = RootSignatureManager::GetInstance()->GetRootSignature(serializedRootSig);

	DescriptorRangePool::GetInstance()->Release();
}

std::tuple<DXGI_FORMAT, size_t> Shader::GetFormatFromReflectionDesc(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask)
{
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	//Component Count
	size_t count = std::ceil(std::log2(mask + 1));

	switch (type)
	{
	case D3D_REGISTER_COMPONENT_UINT32: 
	{
		switch (count)
		{
		case 1:
			format = DXGI_FORMAT_R32_UINT; break;
		case 2:
			format = DXGI_FORMAT_R32G32_UINT; break;
		case 3:
			format = DXGI_FORMAT_R32G32B32_UINT; break;
		case 4:
			format = DXGI_FORMAT_R32G32B32A32_UINT; break;
		default:
			format = DXGI_FORMAT_UNKNOWN;
		}
	}
	break;
	case D3D_REGISTER_COMPONENT_SINT32:
	{
		switch (count)
		{
		case 1:
			format = DXGI_FORMAT_R32_SINT; break;
		case 2:
			format = DXGI_FORMAT_R32G32_SINT; break;
		case 3:
			format = DXGI_FORMAT_R32G32B32_SINT; break;
		case 4:
			format = DXGI_FORMAT_R32G32B32A32_SINT; break;
		default:
			format = DXGI_FORMAT_UNKNOWN;
		}
	}
	break;
	case D3D_REGISTER_COMPONENT_FLOAT32:
	{
		switch (count)
		{
		case 1:
			format = DXGI_FORMAT_R32_FLOAT; break;
		case 2:
			format = DXGI_FORMAT_R32G32_FLOAT; break;
		case 3:
			format = DXGI_FORMAT_R32G32B32_FLOAT; break;
		case 4:
			format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
		default:
			format = DXGI_FORMAT_UNKNOWN;
		}
	}
	break;
	default:
		switch (count)
		{
		case 1:
			format = DXGI_FORMAT_R32_TYPELESS; break;
		case 2:
			format = DXGI_FORMAT_R32G32_TYPELESS; break;
		case 3:
			format = DXGI_FORMAT_R32G32B32_TYPELESS; break;
		case 4:
			format = DXGI_FORMAT_R32G32B32A32_TYPELESS; break;
		default:
			format = DXGI_FORMAT_UNKNOWN;
		}
	}

	return std::make_tuple(format, count * 4);
}

void Shader::BuildInputLayout(ComPtr<ID3DBlob> shader)
{
	mInputLayout.clear();

	ID3D12ShaderReflection* reflection;
	::D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection));

	D3D12_SIGNATURE_PARAMETER_DESC spd;

	int i = 0;
	UINT offset = 0;
	while (SUCCEEDED(reflection->GetInputParameterDesc(i++, &spd)))
	{
		auto [format, size] = GetFormatFromReflectionDesc(spd.ComponentType, spd.Mask);

		mInputLayout.push_back({ spd.SemanticName, 0u, format, 0u, offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u });
		offset += size;
	}
}

//根据当前阶段反射信息，构造mVariables
void Shader::InitInput(ComPtr<ID3DBlob> shader, D3D12_SHADER_VISIBILITY visibility)
{
	if (shader == nullptr)
		return;

	ID3D12ShaderReflection* reflection;
	D3D12_SHADER_INPUT_BIND_DESC ibDesc;

	ThrowIfFailed(::D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection)));

	int j = 0;
	while (true) {
		if (SUCCEEDED(reflection->GetResourceBindingDesc(j++, &ibDesc))) {
			if (mVariables.find(ibDesc.Name) != mVariables.end()) {
				mVariables[ibDesc.Name].Visiblity = D3D12_SHADER_VISIBILITY_ALL;
			}
			else {
				ShaderVariable newVariable;
				newVariable.Name = ibDesc.Name;
				newVariable.BindPoint = ibDesc.BindPoint;
				newVariable.BindCount = ibDesc.BindCount;
				newVariable.Space = ibDesc.Space;
				newVariable.Visiblity = visibility;
				newVariable.Type = ibDesc.Type;

				mVariables[ibDesc.Name] = newVariable;
			}
		}
		else {
			break;
		}
	}

	ID3D12ShaderReflectionConstantBuffer* cbuffer;
	D3D12_SHADER_BUFFER_DESC sbDesc;

	char* lastName = nullptr;
	sbDesc.Name = nullptr;
	j = 0;
	while (true) {
		cbuffer = reflection->GetConstantBufferByIndex(j++);
		cbuffer->GetDesc(&sbDesc);
		if (lastName == sbDesc.Name)
			break;

		if (mConstantBufferDescs.find(sbDesc.Name) == mConstantBufferDescs.end())
		{
			mConstantBufferDescs[sbDesc.Name] = sbDesc;
		}
		lastName = (char*)sbDesc.Name;
	}

	D3D12_SHADER_DESC shaderDesc;
	reflection->GetDesc(&shaderDesc);
	if (visibility == D3D12_SHADER_VISIBILITY_DOMAIN)
	{
		mPrimitiveType = static_cast<D3D_PRIMITIVE_TOPOLOGY>(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + shaderDesc.cControlPoints - 1);
	}
}

UINT Shader::GetSlot(std::string variableName) const {
	if (auto ite = mVariables.find(variableName); ite != mVariables.end())
		return ite->second.rootSlot;
	else {
		//std::string res = "未能根据variable名称找到对应插槽, 未找到的名称为：";
		//res += variableName;
		//std::cout << res << std::endl;
		//throw std::exception(res.c_str());
		return -1;
	}
}

const D3D12_SHADER_BUFFER_DESC* Shader::GetConstantBufferDesc(const std::string& name) const
{
	auto ite = mConstantBufferDescs.find(name);
	if (ite != mConstantBufferDescs.end())
		return &(ite->second);
	else {
		return nullptr;
	}
		
}

void Shader::SetConstantBufferView(ID3D12GraphicsCommandList* cmdList, const std::string& variableName,
	D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) 
{
	UINT Slot = GetSlot(variableName);
	if (Slot != -1)
	{
		if (IsGraphicsShader())
		{
			cmdList->SetGraphicsRootConstantBufferView(Slot, BufferLocation);
		}
		else if (IsComputeShader())
		{
			cmdList->SetComputeRootConstantBufferView(Slot, BufferLocation);
		}

	}
		
}

void Shader::SetTexture(ID3D12GraphicsCommandList* cmdList, const std::string& textureName, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
{
	UINT Slot = GetSlot(textureName);
	if (Slot != -1)
	{
		if (IsGraphicsShader())
		{
			cmdList->SetGraphicsRootDescriptorTable(Slot, BaseDescriptor);
		}
		else if(IsComputeShader())
		{
			cmdList->SetComputeRootDescriptorTable(Slot, BaseDescriptor);
		}
	}
}

//void Shader::SetUnorderAccessView(ID3D12GraphicsCommandList* cmdList, const std::string& variableName, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
//{
//	UINT Slot = GetSlot(variableName);
//	if (Slot != -1)
//	{
//		if (IsGraphicsShader())
//		{
//			cmdList->SetGraphicsRootDescriptorTable(Slot, BaseDescriptor);
//		}
//		else if (IsComputeShader())
//		{
//			cmdList->SetComputeRootDescriptorTable(Slot, BaseDescriptor);
//		}
//	}
//}

void Shader::SetPSODescShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc)
{
	desc->VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
	desc->PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
	if(DS != nullptr) desc->DS = { reinterpret_cast<BYTE*>(DS->GetBufferPointer()), DS->GetBufferSize() };
	if(HS != nullptr) desc->HS = { reinterpret_cast<BYTE*>(HS->GetBufferPointer()), HS->GetBufferSize() };
	if(GS != nullptr) desc->GS = { reinterpret_cast<BYTE*>(GS->GetBufferPointer()), GS->GetBufferSize() };
}

void Shader::SetPSODescTopology(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc)
{
	if (HasTessellationStage())
	{
		desc->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	}
	else
	{
		desc->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
}

bool Shader::RootSignatureEqual(const Shader& lhs, const Shader& rhs)
{
	return lhs.mRootSignature == rhs.mRootSignature;
}

void Shader::BuildComputePipelineState()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.CS =
	{
		reinterpret_cast<BYTE*>(CS->GetBufferPointer()),
		CS->GetBufferSize()
	};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(D3DApp::GetDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&mComputePSO)));
}

}
