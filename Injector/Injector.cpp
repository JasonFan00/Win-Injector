//  Dll injector w/ LoadLibrary

#include <iostream>
#include <Windows.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include <Winnt.h>


DWORD get_pID();
LPCSTR promptDllPath();
LPVOID writePath(HANDLE hProc, LPCSTR dllPath);
FARPROC getLoadLibrary();
bool loadDll(HANDLE hProc, FARPROC loadLibraryAddr, LPVOID addrInTarget);

#pragma comment(lib, "shlwapi.lib")  //  Compiler directive to add static lib for PathFindExtensionA

//  Constants
const LPCSTR FILE_EX_TYPE = ".dll";

int main()
{
	HANDLE hProc;
	LPCSTR dllPath;
	//  Open handle towards target process
	hProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, NULL, get_pID());
	if (hProc == INVALID_HANDLE_VALUE)
	{
		std::cout << "Unable to get handle for process" << std::endl;
		return 1;
	}
	else
	{
		//  Get DLL path
		dllPath = promptDllPath();

		//  Write path to target process
		LPVOID addrInTarget = writePath(hProc, dllPath);
		if (addrInTarget != NULL)
		{
			FARPROC loadLibraryAddr = getLoadLibrary();
			//  Make new thread within target process that executes load library for DLL
			if (loadDll(hProc, loadLibraryAddr, addrInTarget))
			{
				std::cout << "Loaded DLL!" << std::endl;
			}
			else
			{
				std::cerr << "Unable to make new thread in target: " << GetLastError() << std::endl;
			}
		}
	}
	std::cin.clear();
	std::cin.ignore(INT_MAX);
	CloseHandle(hProc);
	std::getchar();
}

//  Get target process ID
DWORD get_pID()
{
	DWORD pID;
	std::cout << "Enter process ID: ";

	do
	{
		std::cin >> pID;
	} while (std::cin.fail());

	return pID;
}

//  Prompt path of DLL from user
LPCSTR promptDllPath()
{
	std::string path;
	std::cout << "Enter the full path of your dll: ";  // C:Documents/... , NOT the path given to you by explorer.  Why?
													   
	std::cin >> path;
	
	LPCSTR ext = PathFindExtensionA(path.c_str());
	DWORD err = GetLastError();
	while (err > 0 || strcmp(ext, FILE_EX_TYPE) || !PathFileExistsA(ext))
	{
		err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND) //  Make sure a file can be found
		{
			std::cout << "Unable to find file";
		}
		if (strcmp(ext, FILE_EX_TYPE))  //  Make sure file extension type matches with .dll
		{
			std::cout << "File extension must end with [" << FILE_EX_TYPE << "], Your extension: " << ext << " ";
		}
		std::cout << "Please re-enter: ";
		std::cin >> path;
	}
	std::cout << "Path: " << path.c_str() << std::endl;
	return path.c_str();
}

LPVOID writePath(HANDLE hProc, LPCSTR dllPath)
{
	//  Reserve then commit memory in external process for path
	LPVOID addrInTarget = VirtualAllocEx(hProc, NULL, sizeof(dllPath), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (addrInTarget != NULL)
	{
		//  Write path to external process to committed memory
		if (WriteProcessMemory(hProc, addrInTarget, &dllPath, sizeof(dllPath), NULL))
		{
			return addrInTarget;
		}
		else
		{
			std::cerr << "Unable to write to process: " << GetLastError() << std::endl;
			return NULL;
		}
	}
	else
	{
		std::cerr << "Unable to reserve/commit memory in target process: " << GetLastError() << std::endl;
		return addrInTarget;
	}
}

FARPROC getLoadLibrary()
{
	HMODULE hKernelModule = GetModuleHandle((L"kernel32.dll"));
	if (hKernelModule != NULL)
	{
		FARPROC loadLibraryAddr = GetProcAddress(hKernelModule, "LoadLibraryA");  // FARPROC - Generic function pointer, LoadLibraryA not LoadLibrary or unable to find function name
		if (loadLibraryAddr != NULL)
		{
			return loadLibraryAddr;
		}
		else
		{
			std::cerr << "Unable to get function address of load library: " << GetLastError() << std::endl;
		}
	}
	else
	{
		std::cerr << "Unable to get kernel module handle: " << GetLastError() << std::endl;
	}
	return NULL;
}

bool loadDll(HANDLE hProc, FARPROC loadLibraryAddr, LPVOID addrInTarget)
{
	HANDLE newThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, addrInTarget, 0, NULL); // LPTHREAD_START_ROUTINE is a function pointer
	if (newThread != NULL)
	{
		CloseHandle(newThread);
		return true;
	}
	else
	{
		return false;
	}
}