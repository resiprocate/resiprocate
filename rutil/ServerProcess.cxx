
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#ifndef WIN32
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <iostream>
#include <fstream>
#include <signal.h>
#include <stdexcept>

#include "rutil/ServerProcess.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::SIP

using namespace resip;
using namespace std;

ServerProcess::ServerProcess() : mPidFile("")
{
}

ServerProcess::~ServerProcess()
{
}

void
ServerProcess::dropPrivileges(const Data& runAsUser, const Data& runAsGroup)
{
#ifdef WIN32
   // setuid is not supported on Windows
   throw std::runtime_error("Unable to drop privileges on Windows, please check the config");
#else
   int rval;
   int new_gid;
   struct passwd *pw;
   struct group *gr;

   if(runAsUser.empty())
      throw std::runtime_error("Unable to drop privileges, username not specified");
   pw = getpwnam(runAsUser.c_str());
   if (pw == NULL)
   {
      throw std::runtime_error("Unable to drop privileges, user not found");
   }

   if(!runAsGroup.empty())
   {
      gr = getgrnam(runAsGroup.c_str());
      if (gr == NULL)
      {
         throw std::runtime_error("Unable to drop privileges, group not found");
      }
      new_gid = gr->gr_gid;
   }
   else
   {
      // Use default group for the specified user
      new_gid = pw->pw_gid;
   }

   rval = getgid();
   if (rval != new_gid)
   {
      if (rval != 0)
      {
         throw std::runtime_error("Unable to drop privileges, not root!");
      }

      rval = setgid(new_gid);
      if (rval < 0)
      {
         throw std::runtime_error("Unable to drop privileges, operation failed");
      }
   }

   rval = getuid();
   if (rval != pw->pw_uid)
   {
      if (rval != 0)
      {
         throw std::runtime_error("Unable to drop privileges, not root!");
      }

      rval = setuid(pw->pw_uid);
      if (rval < 0)
      {
         throw std::runtime_error("Unable to drop privileges, operation failed");
      }
   }
#endif
}

void
ServerProcess::daemonize()
{
#ifdef WIN32
   // fork is not possible on Windows
   throw std::runtime_error("Unable to fork/daemonize on Windows, please check the config");
#else
   pid_t pid;
   if ((pid = fork()) < 0) 
   {
      // fork() failed
      throw std::runtime_error(strerror(errno));
   }
   else if (pid != 0)
   {
      // parent process done
      exit(0);
   }
   if(chdir("/") < 0)
      throw std::runtime_error(strerror(errno));
   // Nothing should be writing to stdout/stderr after this
   close(STDIN_FILENO);
   close(STDOUT_FILENO);
   close(STDERR_FILENO);

   if(mPidFile.size() > 0)
   {
      std::ofstream _pid(mPidFile.c_str(), std::ios_base::out | std::ios_base::trunc);
      _pid << getpid();
      _pid.close();
   }
#endif
}

void
ServerProcess::setPidFile(const Data& pidFile)
{
   mPidFile = pidFile;
}

/* ====================================================================
 *
 * Copyright (c) 2012 Daniel Pocock.  All rights reserved.
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
 */
/*
 * vi: set shiftwidth=3 expandtab:
 */
