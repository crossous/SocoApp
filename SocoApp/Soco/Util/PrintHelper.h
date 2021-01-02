#pragma once

#include "../Shader.h"
#include <iostream>
#include "Redefine.h"
#include "../../Common/magic_enum.hpp"
#include "tool.h"

namespace Soco {

	inline void PrintShaderSummary(Shader* shader) {
		std::cout << "__________________________________" << std::endl;
		std::wcout << "Shader: " << shader->mName << std::endl;
		std::cout << "Shader variable: " << shader->mVariables.size() << std::endl;
		const auto& variables = shader->mVariables;
		for (auto ite = variables.begin(); ite != variables.end(); ++ite) {
			const Shader::ShaderVariable& sv = ite->second;
			std::cout << "\tName: " << sv.Name << std::endl;
			std::cout << "\tType: " << Redefine::enum_name<Redefine::D3D_SHADER_INPUT_TYPE>(sv.Type) << std::endl;
			std::cout << "\tBindPoint: " << sv.BindPoint << std::endl;
			std::cout << "\tBindCount: " << sv.BindCount << std::endl;
			std::cout << "\tSpace: " << sv.Space << std::endl;
			std::cout << "\tRoot Signature Slot: " << sv.rootSlot << std::endl;
			std::cout << "^^^^^" << std::endl;
		}
		std::cout << "__________________________________" << std::endl;
	}
}