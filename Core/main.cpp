#include "main.h"

#include <iostream>
#include <list>
#include <Windows.h>

using namespace std;

static std::list<HWND> m_windowHandles = {};

using fnRtlGetVersion = NTSTATUS(NTAPI*)(PRTL_OSVERSIONINFOEXW lpVersionInformation);
RTL_OSVERSIONINFOEXW GetSystemVersion()
{
	HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
	if (hModule)
	{
		fnRtlGetVersion functionPtr = (fnRtlGetVersion)GetProcAddress(hModule, "RtlGetVersion");
		if (functionPtr != nullptr)
		{
			RTL_OSVERSIONINFOEXW osVersion = { 0 };
			osVersion.dwOSVersionInfoSize = sizeof(osVersion);
			if (functionPtr(&osVersion) == 0)
			{
				return osVersion;
			}
		}
	}
	return { 0 };
}

void ToggleWindowDisplayAffinity(HWND hWnd)
{
	BOOL ret = { 0 };
	static RTL_OSVERSIONINFOEXW pVersionInfo = GetSystemVersion();
	if (pVersionInfo.dwBuildNumber < 19041)
	{
		ret = SetWindowDisplayAffinity(hWnd, WDA_MONITOR);
	}
	else
	{
		ret = SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);
	}

	//if (ret == TRUE)
	//	cout << "SetWindowDisplayAffinity success!" << endl;
	//else
	//	cout << "SetWindowDisplayAffinity failed! GetLastError: " << GetLastError() << endl;
}

BOOL CALLBACK EnumWindowCallback(HWND hWnd, LPARAM lparam)
{
	DWORD processId = NULL;
	GetWindowThreadProcessId(hWnd, &processId);
	if (processId == GetCurrentProcessId())
	{
		m_windowHandles.emplace_back(hWnd);
		ToggleWindowDisplayAffinity(hWnd);
	}
	return TRUE;
}

void Main()
{
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
	SetConsoleOutputCP(CP_UTF8);
	
	cout << "Hello World!" << endl;

	while (true) {
		if (m_windowHandles.empty())
		{
			EnumWindows(EnumWindowCallback, NULL);
		}
		else
		{
			for (auto windowHandle : m_windowHandles)
			{
				ToggleWindowDisplayAffinity(windowHandle);
			}
		}
	}
}