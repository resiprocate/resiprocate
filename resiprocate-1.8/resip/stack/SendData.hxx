#ifndef RESIP_SendData_HXX
#define RESIP_SendData_HXX

#include "rutil/Data.hxx"
#include "resip/stack/Tuple.hxx"

namespace resip
{

/**
   @internal
*/
class SendData
{
   public:
      enum SendDataCommand
      {
         NoCommand,
         CloseConnection,
         EnableFlowTimer
      };

      SendData() : isAlreadyCompressed(false), command(NoCommand)
      {}

      SendData(const Tuple& dest,
               const Data& pdata,
               const Data& tid,
               const Data& scid,
               bool isCompressed = false): 
         destination(dest),
         data(pdata),
         transactionId(tid),
         sigcompId(scid),
         isAlreadyCompressed(isCompressed),
         command(NoCommand)
      {
      }

      // This interface is only used for stun responses
      SendData(const Tuple& dest, char* buffer, int length) : 
         destination(dest),
         data(Data::Take, buffer, length),
         transactionId(Data::Empty),
         sigcompId(Data::Empty),
         isAlreadyCompressed(false),
         command(NoCommand)
      {
      }

      SendData* clone() const
      {
         return new SendData(*this);
      }

      void clear()
      {
         data.clear();
      }

      bool empty() const
      {
         return data.empty();
      }

      Tuple destination;
      Data data;
      Data transactionId;
      Data sigcompId;
      bool isAlreadyCompressed;

      // .bwc. Used for special commands: ie. to close connections, and enable flow timers
      SendDataCommand command;
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
