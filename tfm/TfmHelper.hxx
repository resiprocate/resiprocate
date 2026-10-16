#if !defined TfmHelper_hxx
#define TfmHelper_hxx

// On Windows, make a failed assert() print to stderr and exit, rather than
// open a modal debug CRT dialog: after a console program's assert() message,
// abort() opens an "abort() has been called" dialog.  Under ctest nothing can
// dismiss it, so the run hangs until ctest's timeout instead of failing.  The
// dialogs are kept when a debugger is attached, so it can still break there.
// Does nothing on other platforms, where assert() already aborts.
void reportAssertsToStderr();

#endif
