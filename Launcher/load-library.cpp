#include <Windows.h>
#include <iostream>

#include "load-library.h"

#define ILog(text, ...) printf_s(text, __VA_ARGS__)

bool LoadLibraryDLL(HANDLE hProc, const std::wstring& dllpath)
{
	HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
	if (hKernel == NULL)
	{
		ILog("Error: Failed to get kernel32.dll module address.\n");
		return false;
	}

	LPVOID pLoadLibrary = (LPVOID)GetProcAddress(hKernel, "LoadLibraryW");
	if (pLoadLibrary == NULL)
	{
		ILog("Error: Failed to get LoadLibraryW address.\n");
		return false;
	}

	auto dllpath_size = (wcslen(dllpath.c_str()) + 1) * sizeof(WCHAR);
	LPVOID pDLLPath = VirtualAllocEx(hProc, NULL, dllpath_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (pDLLPath == NULL)
	{
		ILog("Error: Failed to allocate memory for DLLPath in target process.\n");
		return false;
	}

	// Write the string name of our DLL in the memory allocated
	BOOL writeResult = WriteProcessMemory(hProc, pDLLPath, dllpath.c_str(), dllpath_size, NULL);
	if (writeResult == FALSE)
	{
		ILog("Error: Failed to write remote process memory.\n");
		return false;
	}

	// Load our DLL by calling loadlibrary in the other process and passing our dll name
	HANDLE hThread = CreateRemoteThread(hProc, NULL, NULL, (LPTHREAD_START_ROUTINE)pLoadLibrary, (LPVOID)pDLLPath, NULL, NULL);
	if (hThread == NULL)
	{
		ILog("Error: Failed to create remote thread.\n");
		VirtualFreeEx(hProc, pDLLPath, 0, MEM_RELEASE);
		return false;
	}

	// Waiting for thread end and release unnecessary data.
	if (WaitForSingleObject(hThread, 2000) == WAIT_OBJECT_0)
	{
		// ILog("Remote thread ended successfully.\n");
		VirtualFreeEx(hProc, pDLLPath, 0, MEM_RELEASE);
	}

	CloseHandle(hThread);

	ILog("Successfully LoadLibraryW injection.\n");
	return true;
}
