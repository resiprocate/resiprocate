#if !defined(__PRECOMPILE_H__)
#define __PRECOMPILE_H__

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <stdlib.h>
#include <winsock2.h>

#include "rutil/HashMap.hxx"
#include <list>
#include <vector>

#endif

#endif

