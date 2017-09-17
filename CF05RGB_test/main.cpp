#include <iostream>
#include <Windows.h>
#include "CF05RGB.h"

BOOL WINAPI ExitHandler(DWORD signal)
{
	ExitProcess(0);
	return TRUE;
}

int main()
{
	SetConsoleCtrlHandler(ExitHandler, TRUE);

	CF05RGB::Red = 0;
	CF05RGB::Green = 0;
	CF05RGB::Blue = 255;

	std::cout << CF05RGB::Red << ", "
		      << CF05RGB::Green << ", "
	          << CF05RGB::Blue << ", "
			  << CF05RGB::Brightness << "\n";

	for (int j = 0; j < 10; ++j)
	{
		for (int i = 0; i < 255; ++i)
		{
			CF05RGB::Brightness = i;
			CF05RGB::Update();
			Sleep(1);
		}
		for (int i = 255; i >= 0; --i)
		{
			CF05RGB::Brightness = i;
			CF05RGB::Update();
			Sleep(1);
		}
	}

	system("PAUSE");
	return 0;
}