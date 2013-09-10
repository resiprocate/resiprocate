

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

