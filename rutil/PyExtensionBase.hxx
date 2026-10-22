#ifndef PY_EXTENSION_BASE_HXX
#define PY_EXTENSION_BASE_HXX

/* Using the PyCXX API for C++ Python integration
 * It is extremely convenient and avoids the need to write boilerplate
 * code for handling the Python reference counts.
 * It is licensed under BSD terms compatible with reSIProcate */

/* Python C API:
 * https://docs.python.org/3/c-api/init.html
 *
 * PyCXX API:
 * https://cxx.sourceforge.net/PyCXX-Python3.html */

#include <Python.h>
#include <CXX/Objects.hxx>
#include <CXX/Extensions.hxx>

#include "PyThreadSupport.hxx"

namespace resip
{

class PyExtensionBase : public Py::ExtensionModule<PyExtensionBase>
{

   public:
      PyExtensionBase(const resip::Data& exportModuleName);
      virtual ~PyExtensionBase();

      // methods that can be called from Python
      Py::Object logStack(const Py::Tuple &args);
      Py::Object logDebug(const Py::Tuple &args);
      Py::Object logInfo(const Py::Tuple &args);
      Py::Object logWarning(const Py::Tuple &args);
      Py::Object logErr(const Py::Tuple &args);
      Py::Object logCrit(const Py::Tuple &args);

      std::unique_ptr<PyExternalUser> createPyExternalUser();

   protected:
      virtual void initMethods();
      virtual void appendPath(const resip::Data& path);
      virtual bool checkPyArgs(const resip::Data& method, const Py::Tuple &args, unsigned int minArgs, unsigned int maxArgs);
      virtual bool logPythonErrorIfAny();
      virtual std::unique_ptr<Py::Module> loadModule(const resip::Data& moduleName);
      virtual void importModule(PyObject *pyModule, const resip::Data& moduleImport);
      virtual bool init(const resip::Data& pyPath);

      virtual bool onStartup() = 0;

   private:
      const resip::Data mExportModuleName;
      PyThreadState* mThreadState;
      const resip::Data mLogPrefix;
      PyInterpreterState* mInterpreterState;

};

};

#endif

/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2022, Daniel Pocock https://danielpocock.com
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
