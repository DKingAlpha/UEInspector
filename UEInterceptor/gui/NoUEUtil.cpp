#include "NoUEUtil.h"

#include <Windows.h>
#include <debugapi.h>

void LogToConsole(std::string s)
{
	OutputDebugStringA(s.c_str());
}