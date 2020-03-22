
/* standard Net-SNMP includes */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-features.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "rutil/Logger.hxx"
#include "AppSubsystem.hxx"
#include "SNMPThread.hxx"

#include "SNMP_reSIProcate.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::REGISTRATIONAGENT

using namespace registrationagent;
using namespace resip;
using namespace std;

SnmpThread::SnmpThread(const Data& socket)
    : mAccountsTotal(0),
      mAccountsFailed(0),
      mSocket(socket)
{
}

SnmpThread::~SnmpThread()
{
}

void
SnmpThread::thread()
{
   /* Most of the code in this method is copied from the auto-generated
      reSIProcate_subagent.c produced by the command

         mib2c -c subagent.m2c reSIProcate
   */

   /* make us a agentx client. */
   netsnmp_enable_subagent();
   if (!mSocket.empty())
   {
      netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID,
                            NETSNMP_DS_AGENT_X_SOCKET, mSocket.c_str());
   }

   snmp_disable_log();
   /* FIXME if (use_syslog)
      snmp_enable_calllog();
   else
      snmp_enable_stderrlog(); */

   /* initialize tcp/ip if necessary */
   SOCK_STARTUP;

   /* initialize the agent library */
   /* FIXME: use a different name for each reSIProcate application */
   init_agent("reSIProcate");

   /* init reSIProcate mib code */
   init_reSIProcate(&mAccountsTotal, &mAccountsFailed);

   /* read reSIProcate.conf files. */
   /* FIXME: use a different name for each reSIProcate application */
   init_snmp("reSIProcate");

   /* you're main loop here... */
   while(!isShutdown())
   {
      /* if you use select(), see snmp_select_info() in snmp_api(3) */
      /*     --- OR ---  */
      agent_check_and_process(1); /* 0 == don't block */
   }

   snmp_shutdown("reSIProcate");
   SOCK_CLEANUP;
   InfoLog(<<"SnmpThread::thread stopped");
}

void
SnmpThread::shutdown()
{
   if(isShutdown())
   {
      DebugLog(<<"shutdown already in progress!");
      return;
   }
}

void
SnmpThread::setAccountsTotal(const std::size_t& accountsTotal)
{
   mAccountsTotal = accountsTotal;
}

void
SnmpThread::setAccountsFailed(const std::size_t& accountsFailed)
{
   mAccountsFailed = accountsFailed;
}

/* ====================================================================
 *
 * Copyright 2019 Daniel Pocock http://danielpocock.com  All rights reserved.
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
