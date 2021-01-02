#pragma once

//D3D12中有很多enum，有类似ENUM1 = ENUM2的定义行为，不利于magic_enum反射
//这里将对需要的枚举进行重定义

#include <string>
#include "../../Common/magic_enum.hpp"

namespace Soco {
namespace Redefine{
	typedef	enum _D3D_SHADER_INPUT_TYPE_REDEFINE
	{
		D3D_SIT_CBUFFER = 0,
		D3D_SIT_TBUFFER = (D3D_SIT_CBUFFER + 1),
		D3D_SIT_TEXTURE = (D3D_SIT_TBUFFER + 1),
		D3D_SIT_SAMPLER = (D3D_SIT_TEXTURE + 1),
		D3D_SIT_UAV_RWTYPED = (D3D_SIT_SAMPLER + 1),
		D3D_SIT_STRUCTURED = (D3D_SIT_UAV_RWTYPED + 1),
		D3D_SIT_UAV_RWSTRUCTURED = (D3D_SIT_STRUCTURED + 1),
		D3D_SIT_BYTEADDRESS = (D3D_SIT_UAV_RWSTRUCTURED + 1),
		D3D_SIT_UAV_RWBYTEADDRESS = (D3D_SIT_BYTEADDRESS + 1),
		D3D_SIT_UAV_APPEND_STRUCTURED = (D3D_SIT_UAV_RWBYTEADDRESS + 1),
		D3D_SIT_UAV_CONSUME_STRUCTURED = (D3D_SIT_UAV_APPEND_STRUCTURED + 1),
		D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER = (D3D_SIT_UAV_CONSUME_STRUCTURED + 1)
	} 	D3D_SHADER_INPUT_TYPE;

	template <class REDEF_ENUM, class ENUM>
	auto enum_name(ENUM enum_value) {
		return magic_enum::enum_name(static_cast<REDEF_ENUM>(enum_value));
	}

}
}