#include <iostream>
#include <Windows.h>
#include "CF05RGB.h"

static bool running = false;

BOOL WINAPI ExitHandler(DWORD signal)
{
	running = false;
	ExitProcess(0);
	return TRUE;
}

int main()
{
	SetConsoleCtrlHandler(ExitHandler, TRUE);

	CF05RGB::Red = 0;
	CF05RGB::Green = 0;
	CF05RGB::Blue = 0;

	std::cout << CF05RGB::Red << ", "
		      << CF05RGB::Green << ", "
	          << CF05RGB::Blue << ", "
			  << CF05RGB::Brightness << "\n";

	int iRetval = 0;

	while (true)
	{
		for (int i = 0; i < 255; ++i)
		{
			CF05RGB::Brightness = i;
			iRetval = CF05RGB::Update();
			if (iRetval <= 0) CF05RGB::Setup();
			Sleep(1);
		}
		for (int i = 255; i >= 0; --i)
		{
			CF05RGB::Brightness = i;
			iRetval = CF05RGB::Update();
			if (iRetval <= 0) CF05RGB::Setup();
			Sleep(1);
		}
	}

	system("PAUSE");
	return 0;
}