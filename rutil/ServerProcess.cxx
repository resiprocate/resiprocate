
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
#include "rutil/Time.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::SIP

using namespace resip;
using namespace std;

static ServerProcess* _instance = NULL;

static void
signalHandler(int signo)
{
   resip_assert(_instance);
   _instance->onSignal(signo);
}

ServerProcess::ServerProcess() : mPidFile(""),
   mFinished(false),
   mReceivedHUP(false)
{
   resip_assert(!_instance);
   _instance = this;
}

ServerProcess::~ServerProcess()
{
   _instance = NULL;
}

void
ServerProcess::onSignal(int signo)
{
#ifndef _WIN32
   if(signo == SIGHUP)
   {
      InfoLog(<<"Received HUP signal, logger reset");
      Log::reset();
      mReceivedHUP = true;
      return;
   }
#endif
   std::cerr << "Shutting down" << endl;
   mFinished = true;
}

void
ServerProcess::installSignalHandler()
{
   // Install signal handlers
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
   if ( signal( SIGHUP, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGHUP" << endl;
      exit( -1 );
   }
#endif

   if ( signal( SIGINT, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGINT" << endl;
      exit( -1 );
   }

   if ( signal( SIGTERM, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGTERM" << endl;
      exit( -1 );
   }
}

void
ServerProcess::dropPrivileges(const Data& runAsUser, const Data& runAsGroup)
{
#ifdef WIN32
   // setuid is not supported on Windows
   throw std::runtime_error("Unable to drop privileges on Windows, please check the config");
#else
   int rval;
   uid_t cur_uid;
   gid_t cur_gid;
   uid_t new_uid;
   gid_t new_gid;
   const char *username;
   struct passwd *pw;
   struct group *gr;

   if(runAsUser.empty())
   {
      ErrLog(<<"Unable to drop privileges, username not specified");
      throw std::runtime_error("Unable to drop privileges, username not specified");
   }
   username = runAsUser.c_str();
   pw = getpwnam(username);
   if (pw == NULL)
   {
      ErrLog(<<"Unable to drop privileges, user not found");
      throw std::runtime_error("Unable to drop privileges, user not found");
   }
   new_uid = pw->pw_uid;

   if(!runAsGroup.empty())
   {
      gr = getgrnam(runAsGroup.c_str());
      if (gr == NULL)
      {
         ErrLog(<<"Unable to drop privileges, group not found");
         throw std::runtime_error("Unable to drop privileges, group not found");
      }
      new_gid = gr->gr_gid;
   }
   else
   {
      // Use default group for the specified user
      new_gid = pw->pw_gid;
   }

   cur_gid = getgid();
   if (cur_gid != new_gid)
   {
      if (cur_gid != 0)
      {
         ErrLog(<<"Unable to drop privileges, not root!");
         throw std::runtime_error("Unable to drop privileges, not root!");
      }

      rval = setgid(new_gid);
      if (rval < 0)
      {
         ErrLog(<<"Unable to drop privileges, operation failed (setgid)");
         throw std::runtime_error("Unable to drop privileges, operation failed");
      }
   }

   if(initgroups(username, new_gid) < 0)
   {
      ErrLog(<<"Unable to drop privileges, operation failed (initgroups)");
      throw std::runtime_error("Unable to drop privileges, operation failed");
   }

   cur_uid = getuid();
   if (cur_uid != new_uid)
   {
      if (cur_uid != 0)
      {
         ErrLog(<<"Unable to drop privileges, not root!");
         throw std::runtime_error("Unable to drop privileges, not root!");
      }

      // If logging to file, the file ownership may be root and needs to
      // be changed
      Log::droppingPrivileges(new_uid, new_gid);
      if(mPidFile.size() > 0)
      {
         if(chown(mPidFile.c_str(), new_uid, new_gid) < 0)
         {
            ErrLog(<<"Failed to change ownership of PID file");
         }
      }

      rval = setuid(new_uid);
      if (rval < 0)
      {
         ErrLog(<<"Unable to drop privileges, operation failed (setuid)");
         throw std::runtime_error("Unable to drop privileges, operation failed");
      }
   }
#endif
}

bool
ServerProcess::isAlreadyRunning()
{
#ifndef __linux__
   //WarningLog(<<"can't check if process already running on this platform (not implemented yet)");
   return false;
#else
   if(mPidFile.size() == 0)
   {
      // if no PID file specified, we do not make any check
      return false;
   }

   pid_t running_pid;
   std::ifstream _pid(mPidFile.c_str(), std::ios_base::in);
   if(!_pid.good())
   {
      // if the file doesn't exist or can't be opened, just ignore
      return false;
   }
   _pid >> running_pid;
   _pid.close();

   StackLog(<< mPidFile << " contains PID " << running_pid);

   Data ourProc = Data("/proc/self/exe");
   Data otherProc = Data("/proc/") + Data(running_pid) + Data("/exe");
   char our_exe[513], other_exe[513];
   int buf_size;

   buf_size = readlink(ourProc.c_str(), our_exe, 512);
   if(buf_size < 0 || buf_size == 512)
   {
      // if readlink fails, just ignore
      return false;
   }
   our_exe[buf_size] = 0;

   buf_size = readlink(otherProc.c_str(), other_exe, 512);
   if(buf_size < 0 || buf_size == 512)
   {
      // if readlink fails, just ignore
      return false;
   }
   other_exe[buf_size] = 0;

   if(strcmp(our_exe, other_exe) == 0)
   {
      ErrLog(<<"already running PID: " << running_pid);
      return true;
   }
   return false;
#endif
}

void
ServerProcess::daemonize()
{
#ifdef WIN32
   // fork is not possible on Windows
   ErrLog(<<"Unable to fork/daemonize on Windows, please check the config");
   throw std::runtime_error("Unable to fork/daemonize on Windows, please check the config");
#else
   pid_t pid;
   if ((pid = fork()) < 0) 
   {
      // fork() failed
      ErrLog(<<"fork() failed: "<<strerror(errno));
      throw std::runtime_error(strerror(errno));
   }
   else if (pid != 0)
   {
      // parent process done
      exit(0);
   }
   if(chdir("/") < 0)
   {
      ErrLog(<<"chdir() failed: "<<strerror(errno));
      throw std::runtime_error(strerror(errno));
   }
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

void
ServerProcess::mainLoop()
{
   // Main program thread, just waits here for a signal to shutdown
   while (!mFinished)
   {
      doWait();
      if(mReceivedHUP)
      {
         onReload();
         mReceivedHUP = false;
      }
      onLoop();
   }
}

void
ServerProcess::doWait()
{
   sleepMs(1000);
}

void
ServerProcess::onLoop()
{
}

void
ServerProcess::onReload()
{
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
