#ifndef __P2P_Message_hxx
#define __P2P_Message_hxx

#include "rutil/ResipAssert.h"
#include <memory>
#include "rutil/Data.hxx"

#include "p2p/Event.hxx"
#include "p2p/Signable.hxx"
#include "p2p/ResourceId.hxx"
#include "p2p/NodeId.hxx"
#include "p2p/DestinationId.hxx"
#include "p2p/MessageStructsGen.hxx"
#include "p2p/Candidate.hxx"

namespace p2p 
{

class MessageContents;
class ErrorResponse;
class Event;


class JoinAns;
class UpdateAns;
class LeaveAns;
class ConnectAns;

class Message : public Signable
{
   public:
      // Used to make a request. Will populate the rid into the destination list.  
      Message(ResourceId rid);
      
      virtual ~Message() = 0;
      
      enum MessageType
      {
         PingReqType = 1,
         PingAnsType = 2,
         ConnectReqType = 3, //
         ConnectAnsType = 4, //
         TunnelReqType = 5,
         TunnelAnsType = 6,
         StoreReqType = 7, //
         StoreAnsType = 8, //
         FetchReqType = 9, //
         FetchAnsType = 10, //
         RemoveReqType = 11,
         RemoveAnsType = 12,
         FindReqType = 13,
         FindAnsType = 14,
         JoinReqType = 15, //
         JoinAnsType = 16, //
         LeaveReqType = 17,
         LeaveAnsType = 18,
         UpdateReqType = 19, //
         UpdateAnsType = 20, //
         RouteQueryReqType = 21,
         RouteQueryAnsType = 22,
         FailureRequestType = 0xFFFE,
         FailureResponseType = 0xFFFF
      };

      static const UInt8 MessageVersion;
      static const UInt8 MessageTtl;
      static const UInt32 MessageReloToken;


      void setOverlayName(const resip::Data &overlayName);

      struct Error 
      {
            enum Code
            {
               MovedTemporarily = 302,
               Unauthorized = 401,
               Forbidden = 403,
               NotFound = 404,
               RequestTimeout = 408,
               PreconditionFailed = 412,
               IncompatibleWithOverlay = 498
            };
      };
         
      bool isRequest() const;  // convenience function

      virtual MessageType getType() const = 0;

      // copies via list to dest. list in reverse order
      virtual Message *makeErrorResponse(Error::Code code, 
                                         const resip::Data& reason) const;

      JoinAns* makeJoinResponse(const resip::Data &overlaySpecific = resip::Data::Empty);
      UpdateAns* makeUpdateResponse(const resip::Data &overlaySpecific);
      LeaveAns* makeLeaveResponse();
      ConnectAns* makeConnectResponse(const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<Candidate> &candidates);

      resip::Data getRequestMessageBody() const;
      
      // encoding/parsing methods
      resip::Data encodePayload();
      static Message *parse(const resip::Data &message);

      // TTL
      UInt8 getTTL() const;
      void decrementTTL();
      void setTTL(UInt8 ttl);

      UInt32 getOverlay() const;
      UInt64 getTransactionId() const;
      UInt16 getFlags() const; 
      void pushVia(const DestinationId& node);
      void pushDestinationId(const DestinationId& did);

      NodeId getResponseNodeId() const; // pull this from the via list

      // placeholder for doing via list compression
      
      bool isDestinationListEmpty() const;
      DestinationId nextDestination() const;
      void popNextDestinationId(); 
      
      virtual std::auto_ptr<Event> event() = 0;
      
      virtual resip::Data brief() const =0;
      

		void dump() const;

		bool operator==(const Message& msg) const;

protected:
	ResourceId mResourceId;
	resip::Data mOverlayName;
	resip::Data mEncodedData;
	resip::Data mRequestMessageBody;

      s2c::ForwardingLayerMessageStruct mPDU;

      // these have to be defined in the subclasses to encode and decode their message bodies
      virtual void getEncodedPayload(resip::DataStream &dataStream) = 0;
      virtual void decodePayload(resip::DataStream &dataStream) = 0;

      virtual std::vector<resip::Data> collectSignableData() const;

      Message();
		Message(const DestinationId &dest);
      void copyForwardingData(const Message &header);

   private:
      void initForwardingData();
		bool compareDestinationLists(const std::vector<s2c::DestinationStruct *> &l1, const std::vector<s2c::DestinationStruct *> &l2) const;

};

class MessageContents 
{
   public:
};

class ErrorResponse : public Message
{
   public:
      virtual MessageType getType() const { return FailureResponseType; }
      
      Error::Code getErrorCode() const;
      const resip::Data& getReasonPhrase() const;

   protected:
      virtual std::vector<resip::Data> collectSignableData() const;
};

class RouteQueryAns : public Message
{
   public:
      virtual MessageType getType() const { return RouteQueryAnsType; }
};

class RouteQueryReq : public Message
{
   public:
      virtual MessageType getType() const { return RouteQueryReqType; }
};

class RemoveAns : public Message
{
   public:
      virtual MessageType getType() const { return RemoveAnsType; }
};


class RemoveReq : public Message
{
   public:
      virtual MessageType getType() const { return RemoveReqType; }
};


class TunnelAns : public Message
{
   public:
      virtual MessageType getType() const { return TunnelAnsType; }
};


class TunnelReq : public Message
{
   public:
      virtual MessageType getType() const { return TunnelReqType; }
};

class PingAns : public Message
{
   public:
      virtual MessageType getType() const { return PingAnsType; }
};


class PingReq : public Message
{
   public:
      virtual MessageType getType() const { return PingReqType; }
};

class ResourceMessage : public Message
{
   public:
      resip::Data& getResourceId();
      const resip::Data& getResourceId() const;
      void setResourceId(const resip::Data& resourceId);

      resip::Data& getResourceName();
      const resip::Data& getResourceName() const;
      void setResourceName(const resip::Data& resorceName);
      
   private:
      resip::Data mResourceId;
      resip::Data mResourceName;
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
