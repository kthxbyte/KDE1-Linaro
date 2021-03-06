// Error management class
//
// contains error status and last error
//
// Copyright (C) Martin R. Jones 1995
//
// adapted for KSysV by Peter Putzer 1997

using namespace std;

#include <stdlib.h>
#include <iostream>
#include "error.h"

cError Error;
#include <klocale.h>
#include <kapp.h>

// ============================================================================
// The default error handler
//
void DefHandler(int type, const char *theMsg)
{
	if (type == ERR_FATAL)
	{
		cout << klocale->translate("Fatal Error: ") << theMsg << endl;
		exit(1);
	}
	else
	{
		cout << klocale->translate("Warning: ") << theMsg << endl;
	}
}

// ============================================================================
//
cError::cError()
{
	SetHandler(DefHandler);
	type = ERR_NONE;
}

void cError::Set(int theType, const char *theDesc)
{
	type = theType;
	desc = theDesc;

	handler(type, desc);

	if (type == ERR_WARNING) type = ERR_NONE;
}

