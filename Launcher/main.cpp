#include <iostream>
#include <filesystem>
#include <string>

#include <Windows.h>
#include <TlHelp32.h>

#include "load-library.h"
#include <SimpleIni.h>

using namespace std;

static CSimpleIni ini;

DWORD GetProcessIdByName(const wchar_t* name) {
	PROCESSENTRY32W entry = { };
	entry.dwSize = sizeof(PROCESSENTRY32W);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32FirstW(snapshot, &entry) == TRUE) {
		while (Process32NextW(snapshot, &entry) == TRUE) {
			if (_wcsicmp(entry.szExeFile, name) == 0) {
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		}
	}

	CloseHandle(snapshot);
	return NULL;
}

int main(int argc, char* argv[])
{
	ini.SetUnicode();
	ini.LoadFile("cfg.ini");
	current_path(std::filesystem::path(argv[0]).parent_path());

	wchar_t* ProgramName = const_cast<wchar_t*>(ini.GetValue(L"Launcher", L"ProgramName"));

	if (!ProgramName)
	{
		cout << "Failed to read program name, please input program name below." << endl <<
			"Default name: Genshin Impact Cloud Game.exe" << endl;
		ProgramName = new wchar_t[MAX_PATH];
		wcin.getline(ProgramName, MAX_PATH);

		if (ProgramName[0] == '\0')
		{
			ProgramName = const_cast<wchar_t*>(L"Genshin Impact Cloud Game.exe");
			cout << "Using default name." << endl;
		}
	}

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

	while (GetProcessIdByName(ProgramName) == NULL) {
		cout << "Program isn't running. Wait for 3 sec..." << endl;
		Sleep(3000);
	}

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE,
		GetProcessIdByName(ProgramName));
	if (!hProc) {
		cout << "Failed to open process." << endl;

		Sleep(3000); 
		return -1;
	}
	
	if (LoadLibraryDLL(hProc, (std::filesystem::current_path() / "Core.dll").wstring()))
	{
		ini.SetValue(L"Launcher", L"ProgramName", ProgramName);
		ini.SaveFile("cfg.ini");
	}

	CloseHandle(hProc);

	Sleep(3000);
	return 0;
}

#undef EXIT
