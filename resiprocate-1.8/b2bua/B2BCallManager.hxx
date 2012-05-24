
#ifndef __B2BCallManager_h
#define __B2BCallManager_h

#include <fstream>
#include <list>


#include "resip/dum/DialogUsageManager.hxx"

#include "AuthorizationManager.hxx"
#include "B2BCall.hxx"
#include "CDRHandler.hxx"
#include "TaskManager.hxx"

namespace b2bua
{

class B2BCallManager : public TaskManager::RecurringTask {

protected:
  resip::DialogUsageManager& dum;
  AuthorizationManager *authorizationManager;

  std::list<B2BCall *> calls;

  bool stopping;
  bool mustStopCalls;

  CDRHandler& cdrHandler;

public:
  B2BCallManager(resip::DialogUsageManager& dum, AuthorizationManager *authorizationManager, CDRHandler& cdrHandler);
  virtual ~B2BCallManager();

  void setAuthorizationManager(AuthorizationManager *authorizationManager);

  TaskManager::TaskResult doTaskProcessing();

  /**
   * Stop accepting new calls 
   * Shutdown existing calls
   * Blocks until all existing calls stopped
   */
  void stop();
  bool isStopping();

  void onNewCall(MyAppDialog *aLegDialog, const resip::NameAddr& sourceAddr, const resip::Uri& destinationAddr, const resip::Data& authRealm, const resip::Data& authUser, const resip::Data& authPassword, const resip::Data& srcIp, const resip::Data& contextId, const resip::Data& accountId, const resip::Data& baseIp, const resip::Data& controlId);

  void logStats();

};

}

#endif

/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
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

