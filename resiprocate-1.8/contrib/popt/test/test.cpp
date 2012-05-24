// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <popt.h>

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
   const char* logType = "cout";
   const char* logLevel = "INFO";

   struct poptOption table[] = {
      {"log-type",         'l',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logType,        0, "where to send logging messages", "syslog|cerr|cout|file"},
	  {"log-level",        'v',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logLevel,       0, "specify the default log level", "STACK|DEBUG|INFO|WARNING|ERR|NONE"},
      POPT_AUTOHELP 
      { NULL, 0, 0, NULL, 0 }
   };

   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   if (poptGetNextOpt(context) < -1)
   {
      cerr << "Bad command line argument entered" << endl;
      poptPrintHelp(context, stderr, 0);
      exit(-1);
   }
    
	return 0;
}

