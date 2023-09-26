#include "stdafx.h"
#include "SocketHooks.h"
#include <WinSock2.h>

typedef int(WSAAPI* WSAGetLastError_t)(
	void);


WSAGetLastError_t OriginalWSAGetLastError;

int WSAAPI WSAGetLastError_Wrapper(
	void
)
{
	// Do our custom stuff and parameter rewriting
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "New WSAGetLastError", 19, nullptr, nullptr);
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, nullptr, nullptr);

	// Call the original function
	return OriginalWSAGetLastError();
}


void SetupSocketHooks()
{
	// Setup hooks here, see examples below

	OriginalWSAGetLastError = HookFunction("WS2_32.dll", "WSAGetLastError", &WSAGetLastError_Wrapper);
}

