#include "repro/monkeys/SimpleTargetHandler.hxx"

#include "repro/RequestContext.hxx"
#include "repro/ResponseContext.hxx"

namespace repro
{

SimpleTargetHandler::SimpleTargetHandler() :
   Processor("SimpleTargetHandler")
{
}

SimpleTargetHandler::~SimpleTargetHandler()
{
}

Processor::processor_action_t
SimpleTargetHandler::process(RequestContext &rc)
{
   ResponseContext& rsp=rc.getResponseContext();

   std::list<std::list<resip::Data> >& tidBank = rsp.mTransactionQueueCollection;
   std::list<std::list<resip::Data> >::iterator outer=tidBank.begin();
   while(!rsp.hasActiveTransactions() && outer!=tidBank.end())
   {   
      for(; outer!=tidBank.end() && !rsp.hasActiveTransactions(); outer++)
      {
         std::list<resip::Data>::const_iterator i;
         for(i=outer->begin();i!=outer->end();i++)
         {
            rsp.beginClientTransaction(*i);
         }
      }
   }
   
   if(rsp.hasActiveTransactions())
   {
      return Processor::SkipAllChains;
   }
   else
   {
      //If after all this we still don't have any active transactions,
      //make it a free-for-all.
      rsp.beginClientTransactions();
      return Processor::Continue;
   }
}

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
 */
