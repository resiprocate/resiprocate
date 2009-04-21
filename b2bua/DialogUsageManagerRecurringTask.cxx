

#include "resip/dum/DialogUsageManager.hxx"

#include "DialogUsageManagerRecurringTask.hxx"
#include "Logging.hxx"
#include "TaskManager.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

DialogUsageManagerRecurringTask::DialogUsageManagerRecurringTask(resip::SipStack& sipStack, resip::DialogUsageManager& dum) : sipStack(sipStack), dum(dum) {
  stopping = false;
}

TaskManager::TaskResult DialogUsageManagerRecurringTask::doTaskProcessing() {
  FdSet fdset;
  sipStack.buildFdSet(fdset);
  // FIXME - allow time for other tasks
  int err = fdset.selectMilliSeconds(resipMin((int)sipStack.getTimeTillNextProcessMS(), 50));
  if(err == -1) {
    if(errno != EINTR) {
      B2BUA_LOG_ERR("fdset.select returned error code %d", err);
      assert(0);  // FIXME
    }
  }
  // Process all SIP stack activity
  sipStack.process(fdset);
  // Process all DUM activity
  //try {
    while(dum.process()); 
  //} catch(...) {
  //  B2BUA_LOG_ERR("Exception in dum.process(), continuing anyway");
  //}

  // FIXME If sipStack and dum are finished, then we should return TaskDone
  if(!stopping)
    return TaskManager::TaskNotComplete;

  time_t t;
  time(&t);
  if(t > stopTime)
    return TaskManager::TaskIndefinite;
  else
    return TaskManager::TaskNotComplete;
}

void DialogUsageManagerRecurringTask::stop() {
  stopping = true;
  time(&stopTime);
  stopTime += STOP_TIMEOUT;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

