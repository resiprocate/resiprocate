
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/Data.hxx"

namespace resip
{

class ServerProcess
{
public:
   ServerProcess();
   virtual ~ServerProcess();

protected:
   /* The main subclass can call dropPrivileges() if
      if and when it wants to drop root privileges */
   void dropPrivileges(const Data& runAsUser, const Data& runAsGroup);

   /* If the PID file is specified, checks if we are already running
      an instance of this binary */
   bool isAlreadyRunning();

   /* The main subclass can call daemonize() if and
      when it wants to become a daemon */
   void daemonize();

   /* Filename of PID file, or empty string for no PID file */
   void setPidFile(const Data& pidFile);

private:
   Data mPidFile;
};

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
