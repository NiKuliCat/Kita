#include "EnginePch.h"
#include "Log.h"

namespace Kita {



	void Log::Message(const std::string& message)
	{
		std::cout << message.c_str() << std::endl;
	}

}