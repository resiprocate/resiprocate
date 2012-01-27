/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined (HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/LoadLevelTransactionUser.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipStack.hxx"

using namespace resip;

LoadLevelTransactionUser::LoadLevelTransactionUser
        (
          SipStack &stack,
          std::vector<Data> clusterMates,
          unsigned int myClusterNumber
        )
  : mStack(stack), 
    mClusterMates(clusterMates),
    mMyClusterNumber(myClusterNumber)
{
  mName = "LoadLevelTransactionUser";
}

const Data&
LoadLevelTransactionUser::name() const
{
  return mName;
}

void LoadLevelTransactionUser::thread()
{
  while (!isShutdown())
  {
    Message *msg = 0;
    if ((msg = mFifo.getNext(100)) != 0)
    {
      SipMessage *sip = dynamic_cast<SipMessage*>(msg);
      if (sip)
      {
        if (sip->isRequest())
        {
          // Proxy the request to the proper server
          Uri target = findTarget(*sip);
  
          int owner = target.user().hash() % mClusterMates.size();
          if (sip->method() == REGISTER)
          {
            Data registrar = target.scheme();
            registrar += ":";
            registrar += mClusterMates[owner];
            target = Uri(registrar);
          }
          else
          {
            target.host() = mClusterMates[owner];
          }

          // Yes, it's a loose route. Whee!
          target.param(p_lr);

          sip->header(h_Routes).push_back(NameAddr(target));
          sip->header(h_Vias).push_front(Via());

          mStack.send(*sip, this);
        }
        else
        {
          // Proxy the response on
          sip->header(h_Vias).pop_front();

          // CANCEL requests won't have more than one via.
          if (!(sip->header(h_Vias).empty()))
          {
            mStack.send(*sip, this);
          }

          // !abr! TODO: Add code to send ACK if this was a
          //       non-200 response to an INVITE
        }
        delete sip;
      }
    }
  } // of while
} // of thread

LoadLevelTransactionUser::~LoadLevelTransactionUser()
{
}

const Uri &
LoadLevelTransactionUser::findTarget(const SipMessage &msg) const
{
  // For REGISTER requests, the target is in the from header field
  if (msg.method() == REGISTER)
  {
    return msg.header(h_To).uri();
  }

  // Right now, we only use the request URI. There ultimately might
  // be other criteria for this (e.g. if a registrar hands out
  // GRUUs), but that's going to require more thought.
  return msg.header(h_RequestLine).uri();
}

bool
LoadLevelTransactionUser::isForMe(const SipMessage& msg) const
{
    if (msg.isRequest())
    {
        Uri target = findTarget(msg);
        
        int owner = target.user().hash() % mClusterMates.size();
        if (owner == mMyClusterNumber
            || msg.header(h_To).exists(p_tag))
        {
            /* Looks like this request belongs to this element
               in the cluster.
               Say it's not for me, and let the next TU handle
               it. (mjf)
             */
            return false;
        }
    }
    else
    {
      // We have to proxy responses back
      return true;
    }
    /* It's not for this element in the cluster, so answer
       yes, and then we get a chance to 302 it. (mjf)
     */
    return true;
}

/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
