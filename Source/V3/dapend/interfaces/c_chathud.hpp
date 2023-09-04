#pragma once
#include "../interfaces/interfaces.hpp"
#define MAX_BUFFER_SIZE 1024

class c_hudchat {
public:

	void chatprintf(int iPlayerIndex, int iFilter, const char* format, ...)
	{
		static char buf[MAX_BUFFER_SIZE] = "";
		va_list va;
		va_start(va, format);
		vsnprintf_s(buf, MAX_BUFFER_SIZE, format, va);
		va_end(va);
		utilities::call_virtual<void(__cdecl*)(void*, int, int, const char*, ...)>(this, 27)(this, iPlayerIndex, iFilter, buf);
	}
};