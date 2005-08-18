// This is from "sip.mcp - C/C++ Preprocessor - Prefix Text" settings

// set up TARGET_OS_MAC
#include <TargetConditionals.h>

// voodoo magic
#define USE_ARES 1
#define USE_IPV6 1
#define USE_SSL 1

// Darwin defines assert() to use printf, oddly enough
#include <stdio.h>

// trying to get rid of tolower() link conflict
#include <ctype.h>
