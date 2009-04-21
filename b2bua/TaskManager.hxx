
#ifndef __TaskManager_h
#define __TaskManager_h

#include <ctime>
#include <list>

namespace b2bua
{

/**
 * The TaskManager keeps a record of all recurring tasks, and
 * also tasks scheduled to take place at a specified time.
 * It provides an efficient way of giving CPU time to each task.
 * TaskManager is expected to be used with tasks that do not block
 * or run for extended periods of time.
 * TaskManager runs all tasks in a single thread.
 */

class TaskManager {

public:

  typedef enum TaskResult {
    TaskComplete,		// the task doesn't need to run again
    TaskNotComplete,		// the task would like to run again shortly
    TaskIndefinite		// the task can be run again, but doesn't
				// object if the TaskManager stops
  };

  class RecurringTask {
  public:
    virtual ~RecurringTask() {};
    virtual TaskResult doTaskProcessing() = 0;
    virtual void stop() = 0;
  };

  class ScheduledTask {
  public:
    virtual ~ScheduledTask() {};
    virtual void doTaskSchedule() = 0;
  };

  TaskManager();
  
  /**
   * Start the task manager - blocks until complete.
   * Exits when all recurring tasks exit and no scheduled tasks remain.
   */
  void start();
  void addRecurringTask(RecurringTask *t);
  void scheduleTask(ScheduledTask *t, time_t& executionTime);

  void stop();

protected:
  std::list<RecurringTask *> recurringTasks;
  std::list<ScheduledTask *> scheduleTasks;

};

}

#endif

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

