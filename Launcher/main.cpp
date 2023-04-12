#include <iostream>
#include <filesystem>
#include <string>
#include <list>

#include <Windows.h>
#include <TlHelp32.h>

#include "load-library.h"
#include <simpleini/SimpleIni.h>

using namespace std;

static CSimpleIni ini;

list<DWORD> GetProcessIdListByName(const wchar_t* name) {
	PROCESSENTRY32W entry = { };
	entry.dwSize = sizeof(PROCESSENTRY32W);

	list<DWORD> _pidList = { };

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32FirstW(snapshot, &entry) == TRUE) {
		while (Process32NextW(snapshot, &entry) == TRUE) {
			if (_wcsicmp(entry.szExeFile, name) == 0) {
				_pidList.push_back(entry.th32ProcessID);
			}
		}
	}

	CloseHandle(snapshot);
	return _pidList;
}

int main(int argc, char* argv[])
{
	current_path(std::filesystem::path(argv[0]).parent_path());

#if defined(CRAZY_INJECTION) && defined(_DEBUG)
	PROCESSENTRY32W entry = { };
	entry.dwSize = sizeof(PROCESSENTRY32W);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32FirstW(snapshot, &entry) == TRUE) {
		while (Process32NextW(snapshot, &entry) == TRUE) {
			HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
			if (!hProc)
				continue;

			LoadLibraryDLL(hProc, (std::filesystem::current_path() / "Core.dll").wstring());
		}
	}

	CloseHandle(snapshot);
	return 0;
#endif

	ini.SetUnicode();
	ini.LoadFile("cfg.ini");

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

	while (GetProcessIdListByName(ProgramName).empty()) 
	{
		cout << "Program isn't running. Wait for 3 sec..." << endl;
		Sleep(3000);
	}

	bool b_allSuccess = true;
	auto pidList = GetProcessIdListByName(ProgramName);
	for (auto dwPid : pidList)
	{
		HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
		if (!hProc) {
			cout << "Failed to open process. Pid: " << dwPid << endl;

			Sleep(1000);
			continue;
		}

		b_allSuccess = LoadLibraryDLL(hProc, (std::filesystem::current_path() / "Core.dll").wstring());
		CloseHandle(hProc);
	}
	
	if (b_allSuccess)
	{
		ini.SetValue(L"Launcher", L"ProgramName", ProgramName);
		ini.SaveFile("cfg.ini");
	}

	Sleep(3000);
	return 0;
}

#undef EXIT
