#include <memory>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Timer.hxx"

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "Resolver.hxx"
#include "resiprocate/Dialog.hxx"

#include "InviteServer.hxx"
#include "Transceiver.hxx"

using namespace Vocal2;
using namespace Loadgen;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

InviteServer::InviteServer(Transceiver& transceiver)
   : mTransceiver(transceiver)
{}

void
InviteServer::go()
{
   NameAddr contact;
   contact.uri() = mTransceiver.contactUri();
   while (true)
   {
      try
      {
         auto_ptr<SipMessage> invite(waitForRequest(INVITE, 100000));
         contact.uri().user() = invite->header(h_RequestLine).uri().user();
         
         auto_ptr<SipMessage> i_100(Helper::makeResponse(*invite, 100, "Trying"));
         mTransceiver.send(*i_100);

         Data localTag = Helper::computeTag(4);

         auto_ptr<SipMessage> i_180(Helper::makeResponse(*invite, 180, contact, "Ringing"));
         i_180->header(h_To).uri().param(p_tag) = localTag;
         DebugLog(<< "constructed 180: " << *i_180);
         mTransceiver.send(*i_180);

         auto_ptr<SipMessage> i_200(Helper::makeResponse(*invite, 200, contact, "OK"));
         i_200->header(h_To).uri().param(p_tag) = localTag;
         mTransceiver.send(*i_200);
         
         auto_ptr<SipMessage> ack(waitForRequest(ACK, 1000));
         auto_ptr<SipMessage> bye(waitForRequest(BYE, 1000));

         auto_ptr<SipMessage> b_200(Helper::makeResponse(*bye, 200, contact, "OK"));
         mTransceiver.send(*b_200);
      }
      catch(Exception e)
      {
         ErrLog(<< "Proxy not responding.");
         exit(-1);
      }
   }
}

SipMessage* 
InviteServer::waitForResponse(int responseCode,
                              int waitMs)
{
   SipMessage* reg = mTransceiver.receive(waitMs);
   if(reg)
   {         
      if (reg->isResponse() &&
          reg->header(h_StatusLine).responseCode() == responseCode)
      {
         return reg;
      }
      else
      {
         throw Exception("Invalid response.", __FILE__, __LINE__);
      }
   }
   else
   {
      throw Exception("Timed out.", __FILE__, __LINE__);
   }
}

SipMessage* 
InviteServer::waitForRequest(MethodTypes method,
                             int waitMs)
{
   SipMessage* req = mTransceiver.receive(waitMs);
   if(req)
   {         
      if (req->isRequest() &&
          req->header(h_RequestLine).getMethod() == method)
      {
         return req;
      }
      else
      {
         throw Exception("Invalid request.", __FILE__, __LINE__);
      }
   }
   else
   {
      throw Exception("Timed out.", __FILE__, __LINE__);
   }
}

