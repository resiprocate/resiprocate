#ifndef __P2P_TRANSPORTER_MESSAGE_HXX
#define __P2P_TRANSPORTER_MESSAGE_HXX 1

#include "openssl/ssl.h"

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/TransportType.hxx"

#include "p2p/NodeId.hxx"

namespace p2p
{

class ConnectionOpened;
class ConnectionClosed;
class ApplicationMessageArrived;
class ReloadMessageArrived;
class LocalCandidatesCollected;

class ReloadMessage;
class FlowId;
class Candidate;

class TransporterMessage
{
   public:
      virtual ~TransporterMessage() {;}

      virtual ConnectionOpened* castConnectionOpened();
      virtual ConnectionClosed* castConnectionClosed();
      virtual ApplicationMessageArrived* castApplicationMessageArrived();
      virtual ReloadMessageArrived* castReloadMessageArrived();
      virtual LocalCandidatesCollected* castLocalCandidatesCollected();

   protected:

      typedef enum
      {
         ConnectionOpenedType,
         ConnectionClosedType,
         ApplicationMessageArrivedType,
         ReloadMessageArrivedType,
         LocalCandidatesCollectedType
      } MessageType;

      virtual MessageType getMessageType() = 0;
};

class ConnectionOpened : public TransporterMessage
{
   public:
      NodeId getNodeId();
      unsigned short getApplication();
      FlowId getFlowId();
      X509 *getCertificate();
      resip::TransportType getTransportType();

   protected:
      virtual MessageType getMessageType() {return ConnectionOpenedType;}
};

class ConnectionClosed : public TransporterMessage
{
   public:
      NodeId getNodeId();
      unsigned short getApplicationId();

   protected:
      virtual MessageType getMessageType() {return ConnectionClosedType;}
};

class ReloadMessageArrived : public TransporterMessage
{
   public:
      NodeId getNodeId();
      ReloadMessage getReloadMessage();

   protected:
      virtual MessageType getMessageType() {return ReloadMessageArrivedType;}
};

class ApplicationMessageArrived : public TransporterMessage
{
   public:
      FlowId getFlowId();
      resip::Data &getData();

   protected:
      virtual MessageType getMessageType() {return ApplicationMessageArrivedType;}
};

class LocalCandidatesCollected : public TransporterMessage
{
   public:
      std::vector<Candidate> &getCandidates();

   protected:
      virtual MessageType getMessageType() {return LocalCandidatesCollectedType;}
};

}

#endif

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
