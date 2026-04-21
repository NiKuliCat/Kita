#pragma once

namespace Kita {

	enum class Tags
	{
		Default = 0,
		UI,
		Camera,
		Player
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