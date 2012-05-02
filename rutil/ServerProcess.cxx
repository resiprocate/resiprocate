
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#if !defined(WIN32)
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

int
ServerProcess::run(int argc, char *argv[])
{
   return main(argc, argv);
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
 * The Vovida Software License, Version 1.0
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
 */
/*
 * vi: set shiftwidth=3 expandtab:
 */
