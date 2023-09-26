// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "DInput8.h"
#include "Hook.h"
#include "CustomHooks.h"

void Init()
{
	// Load the original dinput8.dll from the system directory
	char DInputDllName[MAX_PATH];
	GetSystemDirectoryA(DInputDllName, MAX_PATH);
	strcat_s(DInputDllName, "\\dinput8.dll");
	DInput8DLL = LoadLibraryA(DInputDllName);
	if (DInput8DLL > (HMODULE)31)
	{
		OriginalFunction = (DirectInput8Create_t)GetProcAddress(DInput8DLL, "DirectInput8Create");
	}
	OutputDebugString(TEXT("Initializing...\n"));
	InitializeHooking();
	OutputDebugString(TEXT("Init Completed.\n"));
}

DWORD WINAPI ThreadMain(LPVOID lpParam)
{
	// Delayed to ensure packed/protected program have fully unpacked & relocate the import table
	Sleep(50000);
	
	OutputDebugString(TEXT("SetupHooks...\n"));
	SetupHooks();
	OutputDebugString(TEXT("SetupHooks Completed.\n"));
	//MessageBox(NULL, L"Success", L"From new thread", MB_OK);
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "Thread Finished\n", 16, nullptr, nullptr);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE Module,
	DWORD  ReasonForCall,
	LPVOID Reserved)
{
	switch (ReasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		{
			curProcId = GetCurrentProcessId();
			hModule = Module;
			//DisableThreadLibraryCalls(Module);
			// Create a console for Debug output
			AllocConsole();
			// Pre-Init
			Init(); // Must not be delayed
			//SetupHooks(); // Can be delayed
			// Create new thread
			hThread = CreateThread(NULL, 0, ThreadMain, NULL, 0, &dwThreadId);
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

