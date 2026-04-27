#pragma once
#include "core/UUID.h"

namespace Kita {

	enum class Tags
	{
		Default = 0,
		UI,
		Camera,
		Player
	};

	enum class Type
	{
		StaticMesh,
		Curve
	};


	struct IDComponent
	{
		UUID ID;
		IDComponent() = default;

		IDComponent(const IDComponent&) = default;
		IDComponent& operator=(const IDComponent&) = default;

		IDComponent(const UUID& uuid)
			:ID(uuid) {}

	};

	class ObjectType
	{
	public:
		ObjectType() = default;
		ObjectType(const Type& type)
			:m_Type(type) {
		};

		const Type& Get() const { return m_Type; }
		Type& Get() { return m_Type; }


		void Set(const Type& type) { m_Type = type; }

	private:
		Type m_Type = Type::StaticMesh;
	};


	class Name
	{
	public:
		Name() = default;
		Name(const std::string& name)
			:m_Name(name) {};

		~Name() = default;
		
		std::string  Get() const { return m_Name; }
		std::string& Get()  { return m_Name; }

		void Set(const std::string& name) { m_Name = name; }
	private:

		std::string m_Name = "new object";
	};






}