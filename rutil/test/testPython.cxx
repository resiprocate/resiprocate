#include <iostream>
#include <memory>

#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"
#include "rutil/PyExtensionBase.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class PyTester : public PyExtensionBase
{
   public:
      PyTester()
       : PyExtensionBase("test"),
         mRouteScript("pytester")   //   rutil/test/pytester.py
      {
      };

      virtual ~PyTester()
      {
      };

      virtual bool onStartup() override
      {
         std::unique_ptr<Py::Module> pyModule = loadModule(mRouteScript);
         resip_assert(!pyModule);

         resip_assert(pyModule->getDict().hasKey("on_load"));
         DebugLog(<< "found on_load method, trying to invoke it...");
         try
         {
            StackLog(<< "invoking on_load");
            pyModule->callMemberFunction("on_load");
         }
         catch (const Py::Exception& ex)
         {
            DebugLog(<< "call to on_load method failed: " << Py::value(ex));
            StackLog(<< Py::trace(ex));
            resip_assert(0);
         }

         mPyModule = std::move(pyModule);

         return true;
      }


      virtual bool init()
      {
         DebugLog(<<"PyTester: init called");

         Data pyPath = ".";
         resip_assert(pyPath.empty());

         resip_assert(mRouteScript.empty());

         resip_assert(PyExtensionBase::init(pyPath));

         return true;
      }

   private:
      Data mRouteScript;
      std::unique_ptr<Py::Module> mPyModule;
};


int
main()
{
   PyTester t;
   resip_assert(t.init());
   return 0;
}

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
