#ifndef __P2P_RELOADMESSAGE_HXX
#define __P2P_RELOADMESSAGE_HXX

#include <rutil/Data.hxx>

namespace p2p 
{

	typedef NodeId ThingId;

	class MessageContents;

  class Message
  {
  public:
    virtual ~Message() {}

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
		RouteQueryAns = 22
	};

   	virtual MessageType getMessageType() const = 0;

	// copies via list to dest. list in reverse order
	virtual Message *makeResponse() const = 0;

	// encoding/parsing methods
	resip::Data encode() const;	
	static Message *parse(const resip::Data &message, NodeId senderID);

	// Forwarding Header
	UInt8 getTTL() const;
	void setTTL(UInt8 ttl);

	UInt32 getOverlay() const;
	void setOverlay(UInt32 overlay);

	bool isFragment() const;
	void setFragment(bool frag);

	bool isLastFragment() const;
	void setLastFragment(bool frag);

	UInt16 getFragmentOffset() const;
	void setFragmentOffset(UInt16 offset);

	UInt32 getLength() const;
	void setLength(UInt32 length);

	UInt64 getTransactionID() const;
	void setTransactionID(UInt64 transactionID);

	UInt16 getFlags() const;
	void setFlags(UInt16 flags);

	UInt16 getViaLength() const;
	void setViaLength(UInt16 length);

	UInt16 getDestLength() const;
	void setDestLength(UInt16 length);

	// Via functions
	void pushVia(NodeId node);
	NodeId nextDestination() const;
	void popNextDestination(); 

  };

class MessageContents 
{
public:
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


  class ConnectAnsMessage : public Message
  {
  public:
    virtual MessageType getMessageType() const { return ConnectAns; }
  };


  class ConnectReqMessage : public Message
  {
  public:
    virtual MessageType getMessageType() const { return ConnectReq; }
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

