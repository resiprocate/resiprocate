#ifndef __P2P_RELOADMESSAGE_HXX
#define __P2P_RELOADMESSAGE_HXX

#include "rutil/Data.hxx"
#include "p2p/Signable.hxx"
#include "p2p/ResourceId.hxx"
#include "p2p/NodeId.hxx"
#include "p2p/MessageStructsGen.hxx"

namespace p2p 
{

class MessageContents;
class ErrorResponse;

enum ErrorResponseCode
{
   EMovedTemporarily = 302,
   EUnauthorized = 401,
   EForbidden = 403,
   ENotFound = 404,
   ERequestTimeout = 408,
   EPreconditionFailed = 412,
   EIncompatibleWithOverlay = 498
};

class Message : public Signable, private s2c::ForwardingHdrStruct
{
   public:
      // Used to make a request. Will populate the rid into the destination list.  
      Message(ResourceId rid, const resip::Data& overlayName);
      
      virtual ~Message()=0;

      enum MessageType
      {
         PingReq = 1,
         PingAns = 2,
         ConnectReq = 3, //
         ConnectAns = 4, //
         TunnelReq = 5,
         TunnelAns = 6,
         StoreReq = 7, //
         StoreAns = 8, //
         FetchReq = 9, //
         FetchAns = 10, //
         RemoveReq = 11,
         RemoveAns = 12,
         FindReq = 13,
         FindAns = 14,
         JoinReq = 15, //
         JoinAns = 16, //
         LeaveReq = 17,
         LeaveAns = 18,
         UpdateReq = 19, //
         UpdateAns = 20, //
         RouteQueryReq = 21,
         RouteQueryAns = 22,
         FailureRequest = 0xFFFE,
         FailureResponse = 0xFFFF
      };

	  bool isRequest() const;  // convenience function

      virtual MessageType getMessageType() const = 0;

      // copies via list to dest. list in reverse order
      virtual Message *makeResponse() const = 0;

      virtual Message *makeErrorResponse(ErrorResponseCode code, 
                                         const resip::Data& reason) const = 0; 
      
      // encoding/parsing methods
      resip::Data encode() const;	
      static Message *parse(const resip::Data &message, NodeId senderID);

      // Forwarding Header
      UInt8 getTTL() const;
      UInt32 getOverlay() const;
      UInt64 getTransactionID() const;
      UInt16 getFlags() const; 
      //void setFlags(UInt16 flags);
      void pushVia(NodeId node);
      // placeholder for doing via list compression
      
      DestinationId nextDestination() const;
      void popNextDestinationId(); 
protected:
	ResourceId mResourceId;
	resip::Data mOverlayName;
	resip::Data mEncodedData;

	virtual void encodePayload(resip::Data &data) const = 0;
};

class MessageContents 
{
   public:
};

class ErrorResponse : public Message
{
   public:
      virtual MessageType getMessageType() const { return FailureResponse; }
      
      ErrorResponseCode getErrorCode() const;
      const resip::Data& getReasonPhrase() const;

   protected:
      virtual std::vector<resip::Data> collectSignableData() const;
};

class ConnectReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return ConnectReq; }
};

class ConnectAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return ConnectAns; }
};

class RouteQueryAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return RouteQueryAns; }
};

class RouteQueryReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return RouteQueryReq; }
};

class LeaveAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return LeaveAns; }
};


class LeaveReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return LeaveReq; }
};

class FindAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return FindAns; }
};


class FindReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return FindReq; }
};


class RemoveAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return RemoveAns; }
};


class RemoveReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return RemoveReq; }
};




class TunnelAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return TunnelAns; }
};


class TunnelReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return TunnelReq; }
};

class PingAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return PingAns; }
};


class PingReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return PingReq; }
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
