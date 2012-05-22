
#include <cassert>
#include <list>

#include "Logging.hxx"
#include "TaskManager.hxx"

using namespace b2bua;
using namespace std;

TaskManager::TaskManager() {
}

void TaskManager::start() {
  while(true) {
    int incompleteCount = 0;
    list<RecurringTask *>::iterator iterator = recurringTasks.begin();
    while(iterator != recurringTasks.end()) {
      // FIXME - read return value, remove completed tasks
      RecurringTask *t = *iterator;
      iterator++;
      TaskResult r = t->doTaskProcessing();
      switch(r) {
        case TaskComplete:
          recurringTasks.remove(t);
          break;
        case TaskNotComplete:
          incompleteCount++;
          break;
        default:
          // ignore any other return value
          break;
      }
    }
    if(incompleteCount == 0) {
      // All tasks are done
      B2BUA_LOG_NOTICE("all tasks complete");
      return;
    }
    // FIXME - do scheduled tasks (not yet implemented)
  }
}

void TaskManager::addRecurringTask(RecurringTask *t) {
  recurringTasks.push_back(t);
}

void TaskManager::scheduleTask(ScheduledTask *t, time_t& executionTime) {
  // FIXME - not yet implemented
  B2BUA_LOG_CRIT("scheduleTask not implemented");
  assert(0);
}

void TaskManager::stop() {
  list<RecurringTask *>::iterator iterator = recurringTasks.begin();
  while(iterator != recurringTasks.end()) {
    RecurringTask *t = *iterator;
    iterator++;
    t->stop();
  }
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

