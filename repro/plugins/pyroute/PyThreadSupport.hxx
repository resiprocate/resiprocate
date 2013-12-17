
#ifndef PYTHREAD_SUPPORT_HXX
#define PYTHREAD_SUPPORT_HXX

#include <Python.h>
#include <CXX/Objects.hxx>

namespace repro
{

class PyThreadSupport
{
   public:
      PyThreadSupport() : mState(PyGILState_Ensure()) {};
      ~PyThreadSupport() { PyGILState_Release(mState); };

   private:
      PyGILState_STATE mState;
};

}

#endif


