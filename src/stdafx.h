// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <Windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifndef GCC
#	include <crtdbg.h>
#endif
#include <tchar.h>
#include <time.h>
#include <assert.h>

#include "msys.h"


// TODO: reference additional headers your program requires here
#define	SIZEOF_CHARS(n)				sizeof(n)/sizeof(n[0])

