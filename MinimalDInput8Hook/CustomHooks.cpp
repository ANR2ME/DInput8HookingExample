#include "stdafx.h"
#include "CustomHooks.h"
#include "FileHooks.h"
#include "SocketHooks.h"

void SetupHooks()
{
	// Setup hooks here, see examples below
	SetupFileHooks();
	SetupSocketHooks();
}

