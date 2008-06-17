#include "p2p/TransporterMessage.hxx"

namespace p2p
{

ConnectionOpened*
TransporterMessage::castConnectionOpened()
{
   if (getMessageType() == ConnectionOpenedType)
   {
      return static_cast<ConnectionOpened*>(this);
   }
   return 0;
}

ConnectionClosed*
TransporterMessage::castConnectionClosed()
{
   if (getMessageType() == ConnectionClosedType)
   {
      return static_cast<ConnectionClosed*>(this);
   }
   return 0;
}

ApplicationMessageArrived*
TransporterMessage::castApplicationMessageArrived()
{
   if (getMessageType() == ApplicationMessageArrivedType)
   {
      return static_cast<ApplicationMessageArrived*>(this);
   }
   return 0;
}

MessageArrived*
TransporterMessage::castMessageArrived()
{
   if (getMessageType() == MessageArrivedType)
   {
      return static_cast<MessageArrived*>(this);
   }
   return 0;
}

}


/* ======================================================================
 *  Copyright (c) 2008, Various contributors to the Resiprocate project
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - The names of the project's contributors may not be used to
 *        endorse or promote products derived from this software without
 *        specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================== */
