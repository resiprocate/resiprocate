#include <memory>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Timer.hxx"

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "Resolver.hxx"
#include "resiprocate/Dialog.hxx"

#include "InviteClient.hxx"
#include "Transceiver.hxx"

using namespace resip;
using namespace Loadgen;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

InviteClient::InviteClient(Transceiver& transceiver, const resip::Uri& proxy, 
                           int firstExtension, int lastExtension, 
                           int numInvites)
   : mTransceiver(transceiver),
     mProxy(proxy),
     mFirstExtension(firstExtension),
     mLastExtension(lastExtension),
     mNumInvites(numInvites)
{
   if (mNumInvites == 0)
   {
      mNumInvites = (mLastExtension - mFirstExtension) / 2;
   }
}

void
InviteClient::go()
{
   int numInvited = 0;
   
   Resolver target(mProxy);

   NameAddr to;
   to.uri() = mProxy;
   
   NameAddr from;
   from.uri() = mProxy;
   
   NameAddr contact;
   contact.uri() = mTransceiver.contactUri();
   
   UInt64 startTime = Timer::getTimeMs();
   InfoLog(<< "Invite client is attempting " << mNumInvites << " calls.");
   while (numInvited < mNumInvites)
   {
      for (int i=mFirstExtension; i < mLastExtension-1 && numInvited < mNumInvites; i+=2)
      {
         contact.uri().user() = Data(i);
         from.uri().user() = Data(i);
         to.uri().user() = Data(i+1);
         
         auto_ptr<SipMessage> invite(Helper::makeInvite(to, from, contact));
         
         mTransceiver.send(target, *invite);
         
         try
         {
            auto_ptr<SipMessage> i_100(waitForResponse(100, 1000));
            auto_ptr<SipMessage> i_180(waitForResponse(180, 1000));
            auto_ptr<SipMessage> i_200(waitForResponse(200, 1000));

            DebugLog(<< "Creating dialog.");
            
            Dialog dlog(contact);

            DebugLog(<< "Creating dialog as UAC.");
            dlog.createDialogAsUAC(*i_200);
            
            DebugLog(<< "making ack.");
            auto_ptr<SipMessage> ack(dlog.makeAck(*invite));
            DebugLog(<< "making bye.");
            auto_ptr<SipMessage> bye(dlog.makeBye());

            DebugLog(<< "Sending ack: << *ack");
            
            mTransceiver.send(*ack);
            mTransceiver.send(*bye);
            auto_ptr<SipMessage> b_200(waitForResponse(200, 1000));
            numInvited++;
         }
         catch(Exception e)
         {
            ErrLog(<< "Proxy not responding.");
            exit(-1);
         }
      }
   }
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout << mNumInvites << " peformed in " << elapsed << " ms, a rate of " 
        << mNumInvites / ((float) elapsed / 1000.0) << " calls per second." << endl;
}

SipMessage* 
InviteClient::waitForResponse(int responseCode,
                              int waitMs)
{
   DebugLog(<< "Waiting for a " << responseCode << " for " << waitMs  << "ms");
   SipMessage* reg = mTransceiver.receive(waitMs);
   DebugLog(<< "Finished waiting for " << responseCode);
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

