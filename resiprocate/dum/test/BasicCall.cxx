\#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"


using namespace resip;

//default, Debug outputs
class TestInviteSessionHandler : public InviteSessionHandler
{
   public:
      /// called when an initial INVITE arrives 
      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onNewSession " << msg.brief());
      }
      
      virtual void onNewSession(ServerInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onNewSession " << msg.brief());
      }
      virtual void onFailure(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onFailure " << msg.brief());
      }
      
      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage& msg, const SdpContents*)
      {
         InfoLog(  << "TestInviteSessionHandler::onEarlyMedia " << msg.brief());
      }

      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onProvisional " << msg.brief());
      }

      virtual void onConnected(ClientInviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onConnected(Uac)" << msg.brief());
      }

      virtual void onConnected(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onConnected(Uas) " << msg.brief());
      }

      virtual void onStaleCallTimeout(ClientInviteSessionHandle)
      {
         InfoLog(  << "TestInviteSessionHandler::onStaleCallTimeout " << msg.brief());
      }

      virtual void onTerminated(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onTerminated " << msg.brief());
      }

      virtual void onReadyToSend(InviteSessionHandle, SipMessage& msg);
      {
         InfoLog(  << "TestInviteSessionHandler::onReadyToSend " << msg.brief());
      }

      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)
      {
         InfoLog(  << "TestInviteSessionHandler::onAnswer" << msg.brief());
      }

      virtual void onOffer(InviteSessionHandle, const SipMessage& msg, const SdpContents*)      
      {
         InfoLog(  << "TestInviteSessionHandler::onOffer " << msg.brief());
      }
      
      virtual void onOfferRejected(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onOfferRejected " << msg.brief());
      }

      virtual void onDialogModified(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onDialogModified " << msg.brief());
      }

      virtual void onInfo(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onInfo" << msg.brief());
      }

      virtual void onRefer(InviteSessionHandle, const SipMessage& msg) 
      {
         InfoLog(  << "TestInviteSessionHandler::onRefer " << msg.brief());
      }

      virtual void onReInvite(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog(  << "TestInviteSessionHandler::onReInvite " << msg.brief());
      }
};

   
      


class TestUac : public TestInviteSessionHandler
{
   public:
      bool* pDone;

      TestUac(bool* pDone) { this->pDone = pDone; }

      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage&)
      {
         InfoLog ( << "TestUac::onProvisional" << msg->brief() << endl);
      }      

      virtual void onConnected(ClientInviteSession::Handle cis, const SipMessage& msg)
      {
         InfoLog ( << "TestUac::onConnected" << msg->brief() << endl);
         cis->send(cis->ackConnection());
      }

      virtual void onTerminated(InviteSession::Handle is, const SipMessage& msg)
      {
         cout << "A thinks a session has been terminated" << endl;
      }
};

class TestUas : public InviteSessionHandler
{

   public:
      bool* pDone;
      time_t* pHangupAt;

      HandlerB(bool* pDone, time_t* pHangupAt) 
      { 
         this.pDone = pDone; 
         this.pHangupAt = pHangupAt;
      }

      void 
      onNewSession(ClientInviteSession::Handle, OfferAnswerType oat, const SipMessage& msg)=0
      {
         cout << "B thinks there's a new INVITE session now" << endl;
      }

      void
      onOffer(ServerInviteSession::Handle sis, const SipMessage& msg )
      {
         cout <<  "B got an offer from A and is trying to accept it" << endl;
         sis->setAnswer(new SdpContents());
         sis->send(sis->accept());
         pHangupAt = time(NULL)+5;
      }
      
      void
      onTerminated(InviteSession::Handle is, const SipMessage& msg)
      {
         cout << "B thinks a session has been terminated" << endl;
      }

      // Normal people wouldn't put this functionality in the handler
      // it's a kludge for this test for right now
      void hangup()
      {
         if (mSis)
         {
            cout << "B is hanging up" << endl;
            mSiS->end();
         }
      }
   private:
      ServerInviteSession::Handle mSis;      
};

int 
main (int argc, char** argv)
{

   SipStack stackA;
   stackA.addTransport(UDP, 5060);
   DialogUsageManager dumA(stackA);
   bool aIsDone = false;
   HandlerA handlerA(&aIsDone);
   dumA.setClientRegistrationHandler(handlerA);
   dumA.setInviteSessionHandler(handlerA);


   SipStack stackB;
   stackB.addTransport(UDP, 5070);
   DialogUsageManager dumB(stackB);
   bool bIsDone= false;
   time_t bHangupAt = 0;
   HandlerB handlerB(&bIsDone, &bHangupAt);
   dumB.setClientRegistrationHandler(handlerB);
   dumB.setInviteSessionHandler(handlerB);

   cout << " Trying to send an INVITE from A to B" << endl;


#if 1 
   NameAddr aor( Uri("sip:cullen@localhost:5070") );
   SipMessage * msg = dumA.makeRegistration(aor);

   // I'm ignoring the DialogSetId that gets returned here
   send(msg); 
#endif

#if 1 
   SipMessage * msg =
     dumA.makeInvSession(new Uri("sip:localhost:5070"),
                         new SdpContents());

   // I'm ignoring the DialogSetId that gets returned here
   send(msg); 
#endif


   while ( (!aIsDone) || (!bIsDone) )
   {
     FdSet fdset;
     // Should these be buildFdSet on the DUM?
     stackA.buildFdSet(fdset);
     stackB.buildFdSet(fdset);
     int err = fdset.selectMilliSeconds(100);
     assert ( err != -1 );
     stackA.process(fdset);
     stackB.process(fdset);

     if (bHangupAt!=0)
     {
       if (time(NULL)>bHangUpAt)
       {
         handlerB.hangup();
       }
     }
   }   

   // How do I turn these things off? For now, we just blow
   // out with all the wheels turning...

}
