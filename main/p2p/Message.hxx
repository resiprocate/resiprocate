#ifndef __P2P_RELOADMESSAGE_HXX
#define __P2P_RELOADMESSAGE_HXX

#include "rutil/Data.hxx"
#include "p2p/Signable.hxx"
#include "p2p/ResourceId.hxx"
#include "p2p/NodeId.hxx"

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

class Message : public Signable
{
   public:
      // Used to make a request. Will populate the rid into the destination
      // list. 
      Message(ResourceId rid, const resip::Data& overlayName);
      
      virtual ~Message()=0;

      enum MessageType
      {
         PingReq = 1,
         PingAns = 2,
         ConnectReq = 3,
         ConnectAns = 4,
         TunnelReq = 5,
         TunnelAns = 6,
         StoreReq = 7,
         StoreAns = 8,
         FetchReq = 9,
         FetchAns = 10,
         RemoveReq = 11,
         RemoveAns = 12,
         FindReq = 13,
         FindAns = 14,
         JoinReq = 15,
         JoinAns = 16,
         LeaveReq = 17,
         LeaveAns = 18,
         UpdateReq = 19,
         UpdateAns = 20,
         RouteQueryReq = 21,
         RouteQueryAns = 22,
         FailureRequest = 0xFFFE,
         FailureResponse = 0xFFFF
      };

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
      virtual std::vector<const resip::Data> collectSignableData() const;
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


class UpdateAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return UpdateAns; }
};


class UpdateReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return UpdateReq; }
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


class JoinAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return JoinAns; }
};


class JoinReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return JoinReq; }
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


class FetchAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return FetchAns; }
};


class FetchReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return FetchReq; }
};


class StoreAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return StoreAns; }
};


class StoreReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return StoreReq; }
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

}

#endif

