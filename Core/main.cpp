﻿#include "main.h"

#include <iostream>
#include <string>
#include <list>
#include <random>
#include <Windows.h>

using namespace std;

static std::list<HWND> m_windowHandles = { };

RTL_OSVERSIONINFOEXW GetSystemVersion()
{
	using fnRtlGetVersion = NTSTATUS(NTAPI*)(PRTL_OSVERSIONINFOEXW lpVersionInformation);

	HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
	if (hModule)
	{
		fnRtlGetVersion functionPtr = (fnRtlGetVersion)GetProcAddress(hModule, "RtlGetVersion");
		if (functionPtr != nullptr)
		{
			RTL_OSVERSIONINFOEXW osVersion = { };
			osVersion.dwOSVersionInfoSize = sizeof(osVersion);
			if (functionPtr(&osVersion) == 0)
			{
				return osVersion;
			}
		}
	}
	return { };
}

wstring GetLastErrorAsString(DWORD errorId = 0)
{
	DWORD errorMessageID = errorId == 0 ? ::GetLastError() : errorId;
	if (errorMessageID == 0)
	{
		return wstring();
	}

	LPWSTR messageBuffer = nullptr;

	size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

	wstring message(messageBuffer, size);

	LocalFree(messageBuffer);

	return message;
}

void ToggleWindowDisplayAffinity(HWND hWnd)
{
	BOOL ret = { };
	static RTL_OSVERSIONINFOEXW pVersionInfo = GetSystemVersion();
	if (pVersionInfo.dwBuildNumber < 19041)
	{
		ret = SetWindowDisplayAffinity(hWnd, WDA_MONITOR);
	}
	else
	{
		ret = SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);
	}

	if (ret == FALSE && hWnd != GetConsoleWindow())
		//wcout << "SetWindowDisplayAffinity failed! Window handle: " << hWnd << ". Error: " << GetLastErrorAsString();
		printf_s("SetWindowDisplayAffinity failed! Window handle: %p, GetLastError: %d.\n", hWnd, GetLastError());
	//else
	//	cout << "SetWindowDisplayAffinity success!" << endl;

	return;
}

void ToggleWindowIcon(HWND hWnd, LPWSTR iconRes)
{
	HANDLE hIcon = LoadImageW(NULL, iconRes,
		IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTCOLOR | LR_DEFAULTSIZE);

	if (hIcon == NULL)
	{
		//wcout << "LoadImageW failed! Error: " << GetLastErrorAsString();
		cout << "LoadImageW failed! Error: " << GetLastError() << endl;
		return;
	}

	SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	//SendMessageW(hWnd, WM_SETICON, ICON_SMALL2, (LPARAM)hIcon);
	
	// Todo: Set icon on taskbar, ICON_BIG don't works for me...
	
	return;
}

BOOL CALLBACK EnumWindowCallback(HWND hWnd, LPARAM lparam)
{
	DWORD processId = NULL;
	GetWindowThreadProcessId(hWnd, &processId);
	if (processId == GetCurrentProcessId())
	{
		m_windowHandles.emplace_back(hWnd);
		ToggleWindowDisplayAffinity(hWnd);
		ToggleWindowIcon(hWnd, IDI_APPLICATION);
	}

	return TRUE;
}

string rand_str()
{
	random_device rd;
	default_random_engine random(rd());

	string buffer = { };
	int len = random() % 100;

	for (int i = 0; i < len; i++) {
		char tmp = { };

		switch (random() % 3)
		{
		case 0:
			tmp = random() % 10 + '0';
			break;
		case 1:
			tmp = random() % 26 + 'a';
			break;
		case 2:
			tmp = random() % 26 + 'A';
			break;
		default:
			i--;
			continue;
		}

		buffer.push_back(tmp);
	}

	return buffer;
}

void MainThread()
{
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
	SetConsoleOutputCP(CP_UTF8); // idk why it doesn't works...

	ios::sync_with_stdio(false);
	cout.tie(nullptr);
	
	cout << "Hello World! Launch timestamp: " << time(NULL) << endl;

	while (true) 
	{
		if (m_windowHandles.empty())
		{
			EnumWindows(EnumWindowCallback, NULL);
		}
		else
		{
			for (auto windowHandle : m_windowHandles)
			{
				ToggleWindowDisplayAffinity(windowHandle);
				ToggleWindowIcon(windowHandle, IDI_APPLICATION);
			}
		}

		SetConsoleTitleA(rand_str().c_str());
		Sleep(1500);
	}
}