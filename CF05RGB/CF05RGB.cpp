//This library uses the Windows HID API to communicate with the RGB LED Controller
//included with the IN-WIN 805 Infinity Chassis.

//Documentation:
//HID: https://msdn.microsoft.com/en-us/library/windows/hardware/mt801981(v=vs.85).aspx
//DLL's: https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
//       https://msdn.microsoft.com/en-us/library/9h658af8.aspx

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <strsafe.h>
#include <hidsdi.h>
#include <SetupAPI.h>
#include <initguid.h>
#include <devpkey.h>
#pragma comment(lib, "Hid.lib")
#pragma comment(lib, "SetupAPI.lib")
#pragma section(".CF05RGB", read, write, shared)

#define HARDWARE_ID L"HID\\VID_FF01&PID_0206&REV_0101&MI_01"

HANDLE Device = INVALID_HANDLE_VALUE;
OVERLAPPED overlapped;

#pragma data_seg(".CF05RGB")

namespace CF05RGB
{
	__declspec(dllexport) int Red = 0;
	__declspec(dllexport) int Green = 0;
	__declspec(dllexport) int Blue = 0;
	__declspec(dllexport) int Brightness = 255;
	__declspec(dllexport) int Update();
}

#pragma data_seg()

//Forward Declarations
BOOL Setup();
BOOL Cleanup();
HRESULT GetDevice(_Out_ LPHANDLE DeviceHandle);

//GetDevicePath() Error Codes
#define E_GET_DEVICE_CLASSES         _HRESULT_TYPEDEF_(0xC0080001L)
#define E_GET_DEVICE_DESC            _HRESULT_TYPEDEF_(0xC0080002L)
#define E_GET_DEVICE_INTERFACE       _HRESULT_TYPEDEF_(0xC0080003L)
#define E_GET_DEVICE_ALLOC_DETAILS   _HRESULT_TYPEDEF_(0xC0080004L)
#define E_GET_DEVICE_ALLOC_PATH      _HRESULT_TYPEDEF_(0xC0080005L)
#define E_GET_DEVICE_COPY_PATH       _HRESULT_TYPEDEF_(0xC0080006L)
#define E_GET_DEVICE_DETAILS         _HRESULT_TYPEDEF_(0xC0080007L)
#define E_GET_DEVICE_CREATE_FILE     _HRESULT_TYPEDEF_(0xC0080008L)
#define E_UPDATE_BAD_DEVICE_HANDLE   -1
#define E_UPDATE_WRITE_FILE          -2
#define E_UPDATE_GET_OVERLAPPED      -3

#ifdef _DEBUG
#include <stdio.h>
#define DebugPrint(...) wprintf(__VA_ARGS__)
#define DisplayMsg(STR) MessageBox(NULL, STR, "IN-WIN CF05RGB", MSGF_DIALOGBOX)

//Produces a string from an error
LPCSTR StringError(HRESULT ErrorCode)
{
	switch (ErrorCode)
	{
		default: return "Unknown Error\n";
		case(E_GET_DEVICE_CLASSES): return "GetDevice() failed to enumerate the device classes\n"; break;
		case(E_GET_DEVICE_DESC): return "GetDevice() failed to get device descriptions\n"; break;
		case(E_GET_DEVICE_INTERFACE): return "GetDevice() failed to get the device interface\n"; break;
		case(E_GET_DEVICE_ALLOC_DETAILS): return "GetDevice() failed to allocate space for device details\n"; break;
		case(E_GET_DEVICE_ALLOC_PATH): return "GetDevice() failed to allocate space for the device path\n"; break;
		case(E_GET_DEVICE_COPY_PATH): return "GetDevice() failed to copy the device path\n"; break;
		case(E_GET_DEVICE_DETAILS): return "GetDevice() failed to get the device details\n"; break;
		case(E_GET_DEVICE_CREATE_FILE): return "GetDevice() failed to create a file for the handle\n"; break;
		case(E_UPDATE_BAD_DEVICE_HANDLE): return "Update() received a bad device handle\n"; break;
		case(E_UPDATE_WRITE_FILE): return "Update() failed to write to the device\n"; break;
		case(E_UPDATE_GET_OVERLAPPED): return "Update() failed to get the number of sent bytes\n"; break;
	}
}
#else
#define DebugPrint(STR)
#define DisplayMsg(STR)
#endif

//DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hDllInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		default: return FALSE; break;
		case(DLL_PROCESS_ATTACH): return Setup(); break;
		case(DLL_THREAD_ATTACH): return Setup(); break;
		case(DLL_PROCESS_DETACH): return Cleanup(); break;
		case(DLL_THREAD_DETACH): return Cleanup(); break;
	}
}

//Initialises the library
BOOL Setup()
{
	Cleanup();
	overlapped.Offset = 0xFFFFFFFF;
	overlapped.OffsetHigh = 0xFFFFFFFF;

	//Get the Device Path of the USB Device
	if (FAILED(GetDevice(&Device)))
	{
		DisplayMsg("No Get Device");
		return TRUE;
	}

	return TRUE;
}

//De-initialises the library
BOOL Cleanup()
{
	if (Device != INVALID_HANDLE_VALUE)
	{
		CloseHandle(Device);
		Device = INVALID_HANDLE_VALUE;
	}
	return TRUE;
}

//Gets a Handle to the HID Device used for this application
HRESULT GetDevice(_Out_ LPHANDLE DeviceHandle)
{
	//Initialise Device List
	GUID hidGUID;
	HidD_GetHidGuid(&hidGUID);
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&hidGUID, NULL, NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE) return E_GET_DEVICE_CLASSES;

	//Iterate through USB Devices
	int i = 0;
	int fd = -1;
	DWORD sz = 0;
	DEVPROPTYPE DevPropType;
	SP_DEVINFO_DATA spDevInfo;
	BOOL bGotMoreDevs = FALSE;
	BOOL bGotProperty = FALSE;
	LPWSTR pDevDesc = nullptr;
	spDevInfo.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	do
	{
		//Retrieve the Device Description if available
		bGotMoreDevs = SetupDiEnumDeviceInfo(hDevInfo, i, &spDevInfo);
		bGotProperty = SetupDiGetDevicePropertyW(hDevInfo, &spDevInfo,
			&DEVPKEY_Device_HardwareIds, &DevPropType,
			(PBYTE)pDevDesc, 0, &sz, 0);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			pDevDesc = (LPWSTR)LocalAlloc(LMEM_FIXED, sz);
			bGotProperty = SetupDiGetDevicePropertyW(hDevInfo, &spDevInfo,
				&DEVPKEY_Device_HardwareIds, &DevPropType,
				(PBYTE)pDevDesc, sz, &sz, 0);	
		}

		//Check if the Device Description matches our application
		if ((bGotProperty) && (wcscmp(pDevDesc, HARDWARE_ID) == 0))
		{
			fd = i;
			LocalFree(pDevDesc);
			break;
		}

		LocalFree(pDevDesc);
		++i;
	} while (bGotMoreDevs != FALSE);
	if (fd == -1)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return E_GET_DEVICE_DESC;
	}

	//Get the Interface of the matching Device
	BOOL bRetval = FALSE;
	SP_DEVICE_INTERFACE_DATA spDevInterface;
	spDevInterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	bRetval = SetupDiEnumDeviceInterfaces(hDevInfo, &spDevInfo,
		&hidGUID, 0, &spDevInterface);
	if (!bRetval)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return E_GET_DEVICE_INTERFACE;
	}

	//Get the size of the Device Path from the Interface
	sz = 0;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pspDevDetails = nullptr;

	bRetval = SetupDiGetDeviceInterfaceDetail(hDevInfo, &spDevInterface,
		pspDevDetails, 0, &sz, NULL);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		//Allocate Space for the Device Details
		pspDevDetails = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_FIXED, sz);
		if (pspDevDetails == nullptr)
		{
			SetupDiDestroyDeviceInfoList(hDevInfo);
			return E_GET_DEVICE_ALLOC_DETAILS;
		}

		//Get the actual Device Path from the Interface
		pspDevDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		bRetval = SetupDiGetDeviceInterfaceDetail(hDevInfo, &spDevInterface,
			pspDevDetails, sz, &sz, NULL);
	}
	else if (!bRetval)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return E_GET_DEVICE_DETAILS;
	}

	//Create a File Handle for the HID API to use
	*DeviceHandle = CreateFile(pspDevDetails->DevicePath, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (*DeviceHandle == INVALID_HANDLE_VALUE)
	{
		LocalFree(pspDevDetails);
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return E_GET_DEVICE_CREATE_FILE;
	}
		
	LocalFree(pspDevDetails);
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return S_OK;
}

//Sends a USB Interrupt Transfer to update the LEDs on the 805i
int CF05RGB::Update()
{
	if (Device == INVALID_HANDLE_VALUE)
		return E_UPDATE_BAD_DEVICE_HANDLE;

	DWORD nBytes = 0;
	BYTE buffer[9] =
	{ 
		0, //ReportID
		BYTE(Red * Brightness / 255),
		BYTE(Green * Brightness / 255),
		BYTE(Blue * Brightness / 255),
		0, 0, 0, 0, 0 //Trailing Report Data
	};

	BOOL bRetval = WriteFile(Device, buffer, 9, NULL, &overlapped);
	if ((!bRetval) && (GetLastError() != ERROR_IO_PENDING))
		return E_UPDATE_WRITE_FILE;

	bRetval = GetOverlappedResult(Device, &overlapped, &nBytes, TRUE);
	if (!bRetval)
		return E_UPDATE_GET_OVERLAPPED;

	return nBytes;
}