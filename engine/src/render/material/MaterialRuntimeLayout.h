#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "asset/Asset.h"
namespace Kita {

	inline constexpr std::string_view MaterialSystemFieldShadingModelID = "_KitaShadingModelID";
	inline constexpr std::string_view MaterialSystemFieldCustomDataCount = "_KitaCustomDataCount";

	struct MaterialUniformMember
	{
		std::string Name;
		MaterialValueType ValueType = MaterialValueType::Float;


		uint32_t Offset = 0;
		uint32_t Size = 0;
	};


	struct MaterialResourceBinding
	{
		std::string Name;
		MaterialValueType ValueType = MaterialValueType::Texture2D;
		uint32_t Set = 1;
		uint32_t Binding = 0;
	};


	// 运行时材质布局：
	// 1. 哪些标量/向量进入统一 UBO
	// 2. 哪些纹理走独立 descriptor binding
	struct MaterialRuntimeLayout
	{
		uint32_t DescriptorSet = 1;
		uint32_t UniformBinding = 0;
		uint32_t UniformBufferSize = 0;

		std::vector<MaterialUniformMember> UniformMembers;
		std::vector<MaterialResourceBinding> ResourceBindings;

		const MaterialUniformMember* FindUniformMember(const std::string& name) const
		{
			for (const MaterialUniformMember& member : UniformMembers)
			{
				if (member.Name == name)
					return &member;
			}
			return nullptr;
		}

		const MaterialResourceBinding* FindResourceBinding(const std::string& name) const
		{
			for (const MaterialResourceBinding& binding : ResourceBindings)
			{
				if (binding.Name == name)
					return &binding;
			}
			return nullptr;
		}
	};
}
