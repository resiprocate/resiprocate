#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"


using namespace resip;


class HandlerA : public InviteSessionHandler, public ClientRegistrationHandler
{

   public:
      bool* pDone;

      HandlerA(bool* pDone) { this->pDone = pDone; }

      // Put some onNewInvSessionStuff here
      void 
      onNewSession(ClientInviteSession::Handle, OfferAnswerType oat, const SipMessage& msg)=0
      {
         cout << "A thinks there's a new INVITE session now" << endl;
      }
         
      void
      onAnswer(ClientInviteSession::Handle cis, const SipMessage& msg)
      {
         cout << "A has an answer(SDP) from B" << endl;
      }

      void
      onConnected(ClientInviteSession::Handle cis, const SipMessage& msg)
      {
         cout << "A thinks it's got a connected session" << endl;
      }

      void
      onTerminated(InviteSession::Handle is, const SipMessage& msg)
      {
         cout << "A thinks a session has been terminated" << endl;
      }

      void onSuccess(ClientRegistration::Handle, const SipMessage& response)
      {
         cout << "Registration worked" << endl;
      }
      
      void onFailure(ClientRegistration::Handle, const SipMessage& response)
      {
         cout << "Registration Failed" << endl;
      }


};

class HandlerB : public InviteSessionHandler, public ClientRegistrationHandler
{

   public:
      bool* pDone;
      time_t* pHangupAt;

      HandlerB(bool* pDone, time_t* pHangupAt) 
      { this.pDone = pDone; this.pHangupAt = pHangupAt;}


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
         sis->accept();
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

      void onRefresh(ServerRegistration::Handle, const SipMessage& reg)
      {
      }

      void onRemoveOne(ServerRegistration::Handle, const SipMessage& reg)
      {
      }
      
      void onRemoveAll(ServerRegistration::Handle, const SipMessage& reg)
      {
      }
      
      void onAdd(ServerRegistration::Handle, const SipMessage& reg)
      {
      }
      
      void onExpired(ServerRegistration::Handle, const NameAddr& contact)
      {
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
