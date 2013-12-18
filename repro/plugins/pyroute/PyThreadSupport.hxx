
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

class PyExternalUser
{
   public:
      PyExternalUser(PyInterpreterState* interpreterState)
       : mInterpreterState(interpreterState),
         mThreadState(PyThreadState_New(mInterpreterState)) {};

   class Use
   {
      public:
         Use(PyExternalUser& user)
          : mUser(user)
         { PyEval_RestoreThread(mUser.getThreadState()); };
         ~Use() { mUser.setThreadState(PyEval_SaveThread()); };
      private:
         PyExternalUser& mUser;
   };

   friend class Use;

   protected:
      PyThreadState* getThreadState() { return mThreadState; };
      void setThreadState(PyThreadState* threadState) { mThreadState = threadState; };

   private:
      PyInterpreterState* mInterpreterState;
      PyThreadState* mThreadState;
};

}

#endif

/* ====================================================================
 *
 * Copyright 2013 Daniel Pocock http://danielpocock.com  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

