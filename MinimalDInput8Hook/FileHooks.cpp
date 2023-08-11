#include "stdafx.h"
#include "FileHooks.h"

typedef HANDLE(WINAPI* CreateFileA_t)(
	LPCSTR                lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile);

typedef HANDLE(WINAPI* CreateFileW_t)(
	LPCWSTR                lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES)
#if (_WIN32_WINNT >= 0x0602)

/*typedef struct _CREATEFILE2_EXTENDED_PARAMETERS {
	DWORD dwSize;
	DWORD dwFileAttributes;
	DWORD dwFileFlags;
	DWORD dwSecurityQosFlags;
	LPSECURITY_ATTRIBUTES lpSecurityAttributes;
	HANDLE hTemplateFile;
} CREATEFILE2_EXTENDED_PARAMETERS, * PCREATEFILE2_EXTENDED_PARAMETERS, * LPCREATEFILE2_EXTENDED_PARAMETERS;*/

typedef HANDLE(WINAPI* CreateFile2_t)(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	DWORD dwCreationDisposition,
	LPCREATEFILE2_EXTENDED_PARAMETERS pCreateExParams
);

CreateFile2_t OriginalCreateFile2;

#endif // _WIN32_WINNT >= 0x0602
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES) */

CreateFileA_t OriginalCreateFileA;
CreateFileW_t OriginalCreateFileW;

HANDLE WINAPI CreateFileA_Wrapper(
	LPCSTR                lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile
)
{
	// Do our custom stuff and parameter rewriting
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), lpFileName, (DWORD)strlen(lpFileName), nullptr, nullptr);
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, nullptr, nullptr);

	// Call the original CreateFileA function
	return OriginalCreateFileA(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}

HANDLE WINAPI CreateFileW_Wrapper(
	LPCWSTR                lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile
)
{
	// Do our custom stuff and parameter rewriting
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), lpFileName, (DWORD)wcslen(lpFileName), nullptr, nullptr);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\n", 1, nullptr, nullptr);

	// Call the original CreateFileW function
	return OriginalCreateFileW(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES)
#if (_WIN32_WINNT >= 0x0602)
HANDLE WINAPI CreateFile2_Wrapper(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	DWORD dwCreationDisposition,
	LPCREATEFILE2_EXTENDED_PARAMETERS pCreateExParams
)
{
	// Do our custom stuff and parameter rewriting
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), lpFileName, (DWORD)wcslen(lpFileName), nullptr, nullptr);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\n", 1, nullptr, nullptr);

	// Call the original CreateFileW function
	return OriginalCreateFile2(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		dwCreationDisposition,
		pCreateExParams);
}
#endif // _WIN32_WINNT >= 0x0602
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES) */

void SetupFileHooks()
{
	// Setup hooks here, see examples below

	OriginalCreateFileA = HookFunction("KERNEL32.dll", "CreateFileA", &CreateFileA_Wrapper);
	OriginalCreateFileW = HookFunction("KERNEL32.dll", "CreateFileW", &CreateFileW_Wrapper);
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES)
#if (_WIN32_WINNT >= 0x0602)
	OriginalCreateFile2 = HookFunction("KERNEL32.dll", "CreateFile2", &CreateFile2_Wrapper);
#endif // _WIN32_WINNT >= 0x0602
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES) */
}

