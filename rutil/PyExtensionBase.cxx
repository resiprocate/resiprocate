
// NOTE: Python.h must be included before any standard headers
// See: https://bugzilla.redhat.com/show_bug.cgi?id=518385

/* Using the PyCXX API for C++ Python integration
 * It is extremely convenient and avoids the need to write boilerplate
 * code for handling the Python reference counts.
 * It is licensed under BSD terms compatible with reSIProcate */
#include <Python.h>
#include <CXX/Objects.hxx>
#include <CXX/Extensions.hxx>

#include <memory>
#include <utility>

#include "rutil/Logger.hxx"

#include "PyExtensionBase.hxx"
#include "PyThreadSupport.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::PYTHON

using namespace resip;

// FIXME "resip"
PyExtensionBase::PyExtensionBase(const resip::Data& exportModuleName)
 : Py::ExtensionModule<PyExtensionBase>(exportModuleName.c_str()),
   mExportModuleName(exportModuleName),
   mThreadState(nullptr),
   mLogPrefix(mExportModuleName)
{
}

PyExtensionBase::~PyExtensionBase()
{
   if(mThreadState)
   {
      PyEval_RestoreThread(mThreadState);
      // this can take time as it de-initializes all Python state
      DebugLog(<<"Calling Py_Finalize");
      Py_Finalize();
      DebugLog(<<"Py_Finalize is done");
   }
}

bool
PyExtensionBase::checkPyArgs(const resip::Data& method, const Py::Tuple &args, unsigned int minArgs, unsigned int maxArgs)
{
   DebugLog(<<"python invoked C method: " << method);
   if(args.size() < minArgs)
   {
      ErrLog(<< method << " called with insufficient arguments");
      return false;
   }
   if(args.size() > maxArgs)
   {
      ErrLog(<< method << " called with excess arguments, only using first argument(s)");
   }
   return true;
}

Py::Object
PyExtensionBase::logStack(const Py::Tuple &args)
{
   if(!checkPyArgs("log_stack", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   const Py::String& text(args[0]);
   StackLog(<< '[' << mLogPrefix << "] " << text);
   return Py::None();
}

Py::Object
PyExtensionBase::logDebug(const Py::Tuple &args)
{
   if(!checkPyArgs("log_debug", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   const Py::String& text(args[0]);
   DebugLog(<< '[' << mLogPrefix << "] " << text);
   return Py::None();
}

Py::Object
PyExtensionBase::logInfo(const Py::Tuple &args)
{
   if(!checkPyArgs("log_info", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   const Py::String& text(args[0]);
   InfoLog(<< '[' << mLogPrefix << "] " << text);
   return Py::None();
}

Py::Object
PyExtensionBase::logWarning(const Py::Tuple &args)
{
   if(!checkPyArgs("log_warning", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   const Py::String& text(args[0]);
   WarningLog(<< '[' << mLogPrefix << "] " << text);
   return Py::None();
}

Py::Object
PyExtensionBase::logErr(const Py::Tuple &args)
{
   if(!checkPyArgs("log_err", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   const Py::String& text(args[0]);
   ErrLog(<< '[' << mLogPrefix << "] " << text);
   return Py::None();
}

Py::Object
PyExtensionBase::logCrit(const Py::Tuple &args)
{
   if(!checkPyArgs("log_crit", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   const Py::String& text(args[0]);
   CritLog(<< '[' << mLogPrefix << "] " << text);
   return Py::None();
}

void
PyExtensionBase::initMethods()
{
   // Initialize the ExtensionModule superclass
   add_varargs_method("log_stack", &PyExtensionBase::logDebug, "log_stack(arglist) = log a stack message");
   add_varargs_method("log_debug", &PyExtensionBase::logDebug, "log_debug(arglist) = log a debug message");
   add_varargs_method("log_info", &PyExtensionBase::logDebug, "log_info(arglist) = log a info message");
   add_varargs_method("log_warning", &PyExtensionBase::logWarning, "log_warning(arglist) = log a warning message");
   add_varargs_method("log_err", &PyExtensionBase::logErr, "log_err(arglist) = log a debug message");
   add_varargs_method("log_crit", &PyExtensionBase::logErr, "log_crit(arglist) = log a debug message");
}

void
PyExtensionBase::appendPath(const resip::Data& path)
{
   PyObject *sys_path = PySys_GetObject("path");
   PyObject *addpath = PyUnicode_FromString(path.c_str());
   PyList_Append(sys_path, addpath);
}

bool
PyExtensionBase::logPythonErrorIfAny()
{
   if (PyErr_Occurred()) {
      Py::Exception ex;
      ErrLog(<< "Python exception: " << Py::value(ex));
      return true;
   }
   return false;
}

void
PyExtensionBase::importModule(PyObject *pyModule, const resip::Data& moduleImport)
{
   PyObject *thisModule = module().ptr();
   PyDict_SetItemString(PyModule_GetDict(pyModule), moduleImport.c_str(), thisModule);
}

std::unique_ptr<Py::Module>
PyExtensionBase::loadModule(const resip::Data& moduleName)
{
   PyObject *_pyModule = PyImport_ImportModule(moduleName.c_str());
   if(!_pyModule)
   {
      ErrLog(<<"Failed to load module "<< moduleName);
      logPythonErrorIfAny();
   }

   // Using the line
   //     import resip
   // in a script fails with an error "No module named 'resip'".
   // Therefore, we force it to be included in the dictionary for the
   // script we are invoking.
   importModule(_pyModule, "resip");

   std::unique_ptr<Py::Module> pyModule(new Py::Module(_pyModule));

   return pyModule;
}

std::unique_ptr<PyExternalUser>
PyExtensionBase::createPyExternalUser()
{
   std::unique_ptr<PyExternalUser> p(new PyExternalUser(mInterpreterState));
   return p;
}

bool
PyExtensionBase::init(const resip::Data& pyPath)
{
   // FIXME: what if there are other Python modules?
   Py_Initialize();
#if PY_VERSION_HEX < 0x03070000
   // Since Python 3.7: GIL is initialzed in Py_Initialize()
   // Since Python 3.9: PyEval_InitThreads() is deprecated
   PyEval_InitThreads();
#endif

   initMethods();
   initialize("reSIProcate SIP stack API callbacks");

   appendPath(pyPath);

   //mThreadState = PyGILState_GetThisThreadState();
   mThreadState = PyThreadState_GET();

#if PY_VERSION_HEX < 0x03090000
   mInterpreterState = mThreadState->interp;
#else
   //mInterpreterState = PyThreadState_GetInterpreter(mThreadState);
   mInterpreterState = PyInterpreterState_Get();
#endif

   if(!onStartup())
   {
      // FIXME - use PyThreadSupport or similar scoped lock mechanism
      PyEval_ReleaseThread(mThreadState);
      return false;
   }

   // FIXME - use PyThreadSupport or similar scoped lock mechanism
   PyEval_ReleaseThread(mThreadState);

   return true;
}


/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2013-2022, Daniel Pocock https://danielpocock.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
