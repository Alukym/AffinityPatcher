#include <iostream>
#include <filesystem>
#include <string>

#include <Windows.h>
#include <TlHelp32.h>

using namespace std;

#define IF_EXIT(a, b) if(a) { cout << b << endl;  Sleep(3000); return -1; }
#define EXIT(a) cout << a << endl; return -1;

DWORD GetProcessIdByName(const wchar_t* name) {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE) {
		while (Process32Next(snapshot, &entry) == TRUE) {
			if (_wcsicmp(entry.szExeFile, name) == 0) {
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		}
	}

	CloseHandle(snapshot);
	return 0;
}

int main()
{
	TOKEN_PRIVILEGES priv;
	ZeroMemory(&priv, sizeof(priv));
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		priv.PrivilegeCount = 1;
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid))
			AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL);

		CloseHandle(hToken);
	}

	DWORD dwProc = GetProcessIdByName(L"Genshin Impact Cloud Game.exe");
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProc);
	IF_EXIT(!hProc, "Failed to open process.");

	filesystem::path currentDllPath = std::filesystem::current_path() / "Core.dll";
	string dllpath = currentDllPath.string();
	//cout << dllpath << endl;

	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	if (hKernel == NULL)
	{
		EXIT("Failed to get kernel32.dll module address.");
		return false;
	}

	LPVOID pLoadLibrary = (LPVOID)GetProcAddress(hKernel, "LoadLibraryA");
	if (pLoadLibrary == NULL) {
		EXIT("Failed to get LoadLibraryA address.");
		return false;
	}

	LPVOID pDLLPath = VirtualAllocEx(hProc, NULL, strlen(dllpath.c_str()) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (pDLLPath == NULL) {
		EXIT("Failed to allocate memory for DLLPath in target process.");
		return false;
	}

	BOOL writeResult = WriteProcessMemory(hProc, pDLLPath, dllpath.c_str(), strlen(dllpath.c_str()), NULL);
	if (writeResult == FALSE) {
		EXIT("Failed to write remote process memory.");
		return false;
	}

	HANDLE hThread = CreateRemoteThread(hProc, NULL, NULL, (LPTHREAD_START_ROUTINE)pLoadLibrary, (LPVOID)pDLLPath, NULL, NULL);
	if (hThread == NULL) {
		EXIT("Failed to create remote thread.");
		VirtualFreeEx(hProc, pDLLPath, 0, MEM_RELEASE);
		return false;
	}

	if (WaitForSingleObject(hThread, 2000) == WAIT_OBJECT_0)
		VirtualFreeEx(hProc, pDLLPath, 0, MEM_RELEASE);

	CloseHandle(hThread);

	cout << "Successfully LoadLibraryA injection." << endl;

	CloseHandle(hProc);

	Sleep(5000);
	return 0;
}