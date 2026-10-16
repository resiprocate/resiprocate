#include "TfmHelper.hxx"

#ifdef _WIN32
#include <crtdbg.h>
#include <stdlib.h>
#include <windows.h>
#endif

void
reportAssertsToStderr()
{
#ifdef _WIN32
   if (IsDebuggerPresent())
   {
      return;
   }
   _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
   _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
   _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
   _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
   _set_error_mode(_OUT_TO_STDERR);
   // and no Windows Error Reporting dialog when abort() follows
   _set_abort_behavior(0, _CALL_REPORTFAULT);
#endif
}
