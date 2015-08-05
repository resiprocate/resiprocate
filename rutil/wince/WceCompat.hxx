#ifndef __WCECOMPAT_H
#define __WCECOMPAT_H

#ifdef UNDER_CE

#include "rutil/ResipAssert.h"
namespace resip
{
wchar_t* ToWString(const char *str);
void FreeWString(wchar_t* wstr);


}	//	namespace resip

#endif UNDER_CE

#endif __WCECOMPAT_H
