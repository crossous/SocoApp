#pragma once
#include <string>
#include <map>
#include <vector>
#include "../Common/d3dUtil.h"
#include <winnt.h>

#include <iostream>

namespace Soco {
	//typedef enum _ShaderStage
	//{
	//	VS = 1,
	//	PS = 2,
	//	DS = 4,
	//	HS = 8,
	//	GS = 16
	//} ShaderStage;

	//DEFINE_ENUM_FLAG_OPERATORS(ShaderStage)


	struct ShaderStage
	{
		const char* vs = nullptr;
		const char* ps = nullptr;
		const char* hs = nullptr;
		const char* ds = nullptr;
		const char* gs = nullptr;
		const char* cs = nullptr;
	};


	class Shader
	{
	public:
		struct ShaderVariable
		{
			std::string Name;
			D3D_SHADER_INPUT_TYPE Type;
			UINT BindPoint;
			UINT BindCount;
			UINT Space;
			D3D12_SHADER_VISIBILITY Visiblity;

			UINT rootSlot = -1;
		};

	public:
		Shader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, ShaderStage stage, std::vector<D3D12_INPUT_ELEMENT_DESC>* inputLayout = nullptr);

		Shader(Shader& other) = delete;
		Shader& operator=(Shader&other) = delete;

		Shader(Shader&& rhs) :
			mName(std::move(rhs.mName)),
			VS(rhs.VS), PS(rhs.PS), HS(rhs.HS), DS(rhs.DS), GS(rhs.GS),
			mRootSignature(rhs.mRootSignature),
			mVariables(std::move(rhs.mVariables)),
			mConstantBufferDescs(std::move(rhs.mConstantBufferDescs)),
			mTextureSlot(std::move(rhs.mTextureSlot)),
			mInputLayout(std::move(rhs.mInputLayout)),
			mPrimitiveType(rhs.mPrimitiveType)
		{
			if (this == &rhs)
				return;

			rhs.VS = nullptr;
			rhs.PS = nullptr;
			rhs.HS = nullptr;
			rhs.DS = nullptr;
			rhs.GS = nullptr;
			rhs.mRootSignature = nullptr;
		}

		Shader& operator= (Shader&& rhs) 
		{
			if (this == &rhs)
				return *this;

			mName = rhs.mName;
			VS = rhs.VS; rhs.VS = nullptr;
			PS = rhs.PS; rhs.PS = nullptr;
			HS = rhs.HS; rhs.HS = nullptr;
			DS = rhs.DS; rhs.DS = nullptr;
			GS = rhs.GS; rhs.GS = nullptr;
			mRootSignature = rhs.mRootSignature; rhs.mRootSignature = nullptr;
			mVariables = std::move(rhs.mVariables);
			mConstantBufferDescs = std::move(rhs.mConstantBufferDescs);

			return *this;
		}

		UINT GetSlot(std::string variableName) const;
		const D3D12_SHADER_BUFFER_DESC* GetConstantBufferDesc(const std::string& name) const;

		bool HasTessellationStage() { return DS != nullptr && HS != nullptr; }
		bool IsComputeShader() { return CS != nullptr; }
		bool IsGraphicsShader() { return VS != nullptr && PS != nullptr; }

		void SetConstantBufferView(ID3D12GraphicsCommandList* cmdList, const std::string& variableName, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
		void SetTexture(ID3D12GraphicsCommandList* cmdList, const std::string& textureName, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
		//void SetUnorderAccessView(ID3D12GraphicsCommandList* cmdList, const std::string& variableName, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

		//Graphics Setup
		void SetGraphicsRootSignature(ID3D12GraphicsCommandList* cmdList) { assert(IsGraphicsShader()); cmdList->SetGraphicsRootSignature(mRootSignature.Get());}
		void SetIASetPrimitiveTopology(ID3D12GraphicsCommandList* cmdList){ assert(IsGraphicsShader()); cmdList->IASetPrimitiveTopology(mPrimitiveType); }

		//Setup Compute Shader
		void SetComputeRootSignature(ID3D12GraphicsCommandList* cmdList) { assert(IsComputeShader()); cmdList->SetComputeRootSignature(mRootSignature.Get()); }
		void SetComputePipelineState(ID3D12GraphicsCommandList* cmdList) { assert(IsComputeShader()); cmdList->SetPipelineState(mComputePSO.Get()); }

		//Initialize Graphics PSO Desc
		void SetPSODescShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);
		void SetPSODescTopology(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);
		void SetPSODescRootSignature(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc) { desc->pRootSignature = mRootSignature.Get(); }
		void SetPSODescInputLayout(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc) { desc->InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() }; }


		const std::map<std::string, UINT>& GetTextureSlot() { return mTextureSlot; }

		friend void PrintShaderSummary(Shader* shader);
		//friend class Material;

		static bool RootSignatureEqual(const Shader& lhs, const Shader& rhs);

	private:
		void BuildRootSignature();

		static std::tuple<DXGI_FORMAT, size_t> GetFormatFromReflectionDesc(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask);
		void BuildInputLayout(Microsoft::WRL::ComPtr<ID3DBlob> shader);
		void BuildComputePipelineState();
		void InitInput(Microsoft::WRL::ComPtr<ID3DBlob> shader, D3D12_SHADER_VISIBILITY visibility);

	private:
		std::wstring mName;

		Microsoft::WRL::ComPtr<ID3DBlob> VS;
		Microsoft::WRL::ComPtr<ID3DBlob> PS;
		Microsoft::WRL::ComPtr<ID3DBlob> HS;
		Microsoft::WRL::ComPtr<ID3DBlob> DS;
		Microsoft::WRL::ComPtr<ID3DBlob> GS;

		Microsoft::WRL::ComPtr<ID3DBlob> CS;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> mComputePSO;

		std::map<std::string, ShaderVariable> mVariables;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
		std::map<std::string, D3D12_SHADER_BUFFER_DESC> mConstantBufferDescs;
		std::map<std::string, UINT> mTextureSlot;
		std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

		//当初始化检测到存在DomainShader时，会改变为控制点图元
		D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	};

}

