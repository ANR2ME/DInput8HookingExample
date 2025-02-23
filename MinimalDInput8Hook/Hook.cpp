#include "stdafx.h"
#include "Hook.h"
#include <winternl.h>

typedef NTSTATUS(NTAPI* TFNNtQueryInformationProcess)(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);
TFNNtQueryInformationProcess pfnNtQueryInformationProcess = nullptr;
PROCESS_BASIC_INFORMATION BasicProcessInfo;

int InitInternals() {
	pfnNtQueryInformationProcess = (TFNNtQueryInformationProcess)GetProcAddress(GetModuleHandle(TEXT("Ntdll.dll")), "NtQueryInformationProcess");
	if (!pfnNtQueryInformationProcess) {
		// Experimental only, should probably print this on the debug output and fallback to another version of NtQueryInformationProcess (if any)
		MessageBox(GetConsoleWindow(), TEXT("Failed to get NtQueryInformationProcess"), TEXT("Error"), MB_OK);
		return -1;
	}

	return 0;
}

int InitializeHooking()
{
	if (InitInternals() < 0) {
		return -1;
	}

	// Get our Process Handle
	const HANDLE Process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, GetCurrentProcessId());

	// Find out our base address, where the Executable image is loaded in memory
	//NtQueryInformationProcess(Process, ProcessBasicInformation, &BasicProcessInfo, sizeof(BasicProcessInfo), nullptr);
	// Use dynamically linked internal APIs as it's subject to change from one release of Windows to the next.
	pfnNtQueryInformationProcess(Process, ProcessBasicInformation, &BasicProcessInfo, sizeof(BasicProcessInfo), nullptr);

	return 0;
}

void* HookFunction_Internal(const char* DLLName, const char* FunctionName, void* NewAddress)
{
	// In the PEB structure provided by Microsoft the ImageBaseAddress is part of a "Reserved" array,
	// for better clarity we use a custom struct here
	CUSTOM_PEB* PEBExtendedInfo = (CUSTOM_PEB*)BasicProcessInfo.PebBaseAddress;
	PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)PEBExtendedInfo->ImageBaseAddress;
	// This is the start of the executable file in memory
	BYTE* BaseAddress = (BYTE*)PEBExtendedInfo->ImageBaseAddress;

	// Get the Address of our NT Image headers
	PIMAGE_NT_HEADERS FileHeader = (PIMAGE_NT_HEADERS)(BaseAddress + pDOSHeader->e_lfanew);

	// Retrieve the import directory in which all imported dlls and functions are listed
	IMAGE_DATA_DIRECTORY ImportDirectory = FileHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	const int NumImportDesciptors = ImportDirectory.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
	PIMAGE_IMPORT_DESCRIPTOR Descriptors = (PIMAGE_IMPORT_DESCRIPTOR)(BaseAddress + ImportDirectory.VirtualAddress);

	//WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), DLLName, (DWORD)strlen(DLLName), nullptr, nullptr);
	//WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, nullptr, nullptr);

	// Go through all imported DLLs and find the one we are insteresed in
	for (int ImportDLLIndex = 0; ImportDLLIndex < NumImportDesciptors; ++ImportDLLIndex)
	{
		PIMAGE_IMPORT_DESCRIPTOR Descriptor = Descriptors + ImportDLLIndex;
		const char* ImportDLLName = (const char*)BaseAddress + Descriptor->Name;
		// On a packed/protected program, some of the descriptors might contains an invalid data (feels like exceeding the actual number of import descriptors), zero-filled Descriptor seems to be used as terminator.
		unsigned char* mm = (unsigned char*)Descriptor;
		if ((*mm == 0) && memcmp(mm, mm + 1, sizeof(IMAGE_IMPORT_DESCRIPTOR) - 1) == 0) 
			break;

		//WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), ImportDLLName, (DWORD)strlen(ImportDLLName), nullptr, nullptr);
		//WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, nullptr, nullptr);

		// Check if we found our DLL in the import table
		if (!strcmp(ImportDLLName, DLLName))
		{
			PIMAGE_THUNK_DATA ImportNameTable = (PIMAGE_THUNK_DATA)(BaseAddress + Descriptor->OriginalFirstThunk);
			int Offset = 0;
			// The import name table is a null terminated array, so iterate until we either found it or reach the null termination
			while (ImportNameTable->u1.AddressOfData != 0)
			{
				PIMAGE_IMPORT_BY_NAME NameImport = (PIMAGE_IMPORT_BY_NAME)(BaseAddress + ImportNameTable->u1.AddressOfData);
				// Null terminated function name start pointer is stored in here
				const char* ImportFunctionName = NameImport->Name;
				if (!strcmp(FunctionName, ImportFunctionName))
				{
					PIMAGE_THUNK_DATA ImportAddressTable = (PIMAGE_THUNK_DATA)(BaseAddress + Descriptor->FirstThunk);
					// The import address table is in the same order as the import name table
					ImportAddressTable += Offset;

					void* OriginalAddress = (void*)ImportAddressTable->u1.AddressOfData;
					DWORD OldProtection;
					// Make the page writable to patch the pointer
					VirtualProtect(ImportAddressTable, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &OldProtection);
					ImportAddressTable->u1.AddressOfData = (ULONGLONG)NewAddress;
					// Restore page protection to the previous state
					VirtualProtect(ImportAddressTable, sizeof(IMAGE_THUNK_DATA), OldProtection, &OldProtection);
					return OriginalAddress;
				}


				++ImportNameTable;
				++Offset;
			}

			// We've found the DLL but not the function, break here. No need to go through the rest of the DLLs
			break;
		}
	}

	// DLL and function import not found return nullptr to signal this
	return nullptr;
}

