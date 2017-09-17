#pragma once
#pragma comment(lib, "CF05RGB.lib")

//This library uses the Windows HID API to communicate with the RGB LED Controller
//included with the IN-WIN 805 Infinity Chassis.

namespace CF05RGB
{
	__declspec(dllimport) int Red;
	__declspec(dllimport) int Green;
	__declspec(dllimport) int Blue;
	__declspec(dllimport) int Brightness;
	__declspec(dllimport) int Update();
}