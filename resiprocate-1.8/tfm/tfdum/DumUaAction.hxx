#if !defined(TFM_DumUaAction_hxx)
#define TFM_DumUaAction_hxx

#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "resip/dum/ServerPagerMessage.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "resip/dum/ClientOutOfDialogReq.hxx"
#include "resip/dum/ServerOutOfDialogReq.hxx"
#include "resip/dum/Handles.hxx"
#include "resip/dum/DumCommand.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "tfm/ActionBase.hxx"
#include "tfm/TestEndPoint.hxx"
#include <boost/function.hpp>
#include "rutil/SharedPtr.hxx"

#include "DumEvent.hxx"
#include "tfm/CommonAction.hxx"

class DumUserAgent;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

class DumUaAction : public ActionBase
{
   public:
      explicit DumUaAction(DumUserAgent* tua) : mUa(tua) {}           
      virtual ~DumUaAction() {}
      virtual void operator()(boost::shared_ptr<Event> event);
      virtual void operator()();
      virtual void operator()(DumUserAgent& tua) = 0;
      virtual void operator()(DumUserAgent& tua,boost::shared_ptr<DumEvent> event);
      
   protected:
      DumUserAgent* mUa;
};

class Start : public DumUaAction
{
   public:
      explicit Start(DumUserAgent& dua);
      virtual ~Start() {}
      virtual resip::Data toString() const;
      virtual void operator()(DumUserAgent& dua);
};

class Shutdown : public DumUaAction
{
   public:
      explicit Shutdown(DumUserAgent& dua);
      virtual ~Shutdown() {}
            
      virtual resip::Data toString() const;
      virtual void operator()(DumUserAgent& dua);
};

template<class T, class E, class H>
class UsageAction : public DumUaAction
{
   public:
      typedef boost::function<void (T& c) > Functor;
      typedef H UsageHandleType;      
      
      UsageAction(DumUserAgent* dua, Functor func) :
         DumUaAction(dua),
         mFunctor(func)
      {}      

      virtual void operator()(DumUserAgent& dua, boost::shared_ptr<Event> event)
      {
         E* specificEvent = dynamic_cast<E*>(event.get());
         
         if (specificEvent)
         {
            //UsageHandleType& handle = specificEvent->mHandle;
            UsageHandleType handle = getHandleFromEvent(specificEvent);            
            if(!handle.isValid())
            {
               ErrLog(<< getName() << "::operator(), invalid handle(from DUM)");
               throw TestEndPoint::AssertException("requires a valid handle", __FILE__, __LINE__);
            }
            (*this)(handle);
         }
         else
         {
            (*this)(dua);
         }
      }

   protected:      
      virtual void operator()(DumUserAgent& dua)
      {
         
         if (getHandleFromDua().isValid())
         {
            (*this)(getHandleFromDua());
         }
         else
         {
            ErrLog(<< getName() << "::operator(), invalid handle(from DumUserAgent)");
            throw TestEndPoint::AssertException("requires a vaild handle", __FILE__, __LINE__);
         }
      }
      
      virtual void operator()(H handle)
      {
         StackLog(<< getName() << "::operator(): Executing deferred call: ");
         mFunctor(*handle);      
      }

      virtual resip::Data getName() = 0;
      virtual UsageHandleType getHandleFromDua() = 0;
      virtual UsageHandleType getHandleFromEvent(E* event) = 0;

      Functor mFunctor;
}; 

class MessageAdorner
{
   public:
      virtual ~MessageAdorner() {}
      //virtual resip::SipMessage& operator()(resip::SipMessage&)=0;
      virtual resip::SharedPtr<resip::SipMessage> operator()(resip::SharedPtr<resip::SipMessage>)=0;
};

class NoAdornment : public MessageAdorner
{
   public:
      virtual resip::SharedPtr<resip::SipMessage> operator()(resip::SharedPtr<resip::SipMessage> msg)
      {
         return msg;
      }
      static NoAdornment& instance()
      {
         static NoAdornment noAdornment;
         return noAdornment;
      }
};

class ReferAdornment : public MessageAdorner
{
   public:
      ReferAdornment(const resip::NameAddr& referTo) : mReferTo(referTo) {}
      virtual resip::SharedPtr<resip::SipMessage> operator()(resip::SharedPtr<resip::SipMessage> msg)
      {
         msg->header(resip::h_ReferTo) = mReferTo;
         msg->header(resip::h_ReferSub).value() = "false";
         return msg;
      }
   private:
      resip::NameAddr mReferTo;
};

class ReferAdornmentRemoveReferSubHeader : public MessageAdorner
{
   public:
      ReferAdornmentRemoveReferSubHeader(const resip::NameAddr& referTo) : mReferTo(referTo) {}
      virtual resip::SharedPtr<resip::SipMessage> operator()(resip::SharedPtr<resip::SipMessage> msg)
      {
         msg->header(resip::h_ReferTo) = mReferTo;
         msg->remove(resip::h_ReferSub);
         msg->header(resip::h_Requires).push_back(resip::Token("norefersub"));
         return msg;
      }
   private:
      resip::NameAddr mReferTo;
};

template<class H>
class SendingAction : public ActionBase
{
   public:
      typedef boost::function<resip::SharedPtr<resip::SipMessage> () > Functor;
      
      SendingAction(TestEndPoint* tua, H& handle, resip::Data action, Functor f, MessageAdorner& adorner)
         : mTestEndPoint(tua), 
           mHandle(handle), 
           mActionName(action), 
           mFunctor(f),
           mMessageAdorner(&adorner)
      {
      }

      virtual void operator()(boost::shared_ptr<Event> event)
      {
         (*this)();
      }

      virtual void operator()()
      {
         StackLog(<< "SendingActon");
         StackLog(<< "handle id: " << mHandle.getId());
         if (mHandle.isValid())
         {
            StackLog(<< "sending adorned message...");
            mHandle->sendCommand((*mMessageAdorner)(mFunctor()));
         }
         else
         {
            ErrLog(<< "Invalid handle from usage");
            throw TestEndPoint::AssertException("invalid handle from usage", __FILE__, __LINE__);
         }
      }

      virtual resip::Data toString() const { return mActionName; }

   private:
      TestEndPoint* mTestEndPoint;
      H& mHandle;
      resip:: Data mActionName;
      Functor mFunctor;
      MessageAdorner* mMessageAdorner;
};

template<class T, class E, class H>
class SendingUsageAction : public DumUaAction
{
   public:
      typedef boost::function<void (T& c) > Functor;
      //typedef boost::function<resip::SipMessage& (T& c) > MessageFunctor;
      typedef boost::function<resip::SharedPtr<resip::SipMessage> (T& c) > MessageFunctor;
      typedef H UsageHandleType;      
      
      SendingUsageAction(DumUserAgent* dua, Functor func) :
         DumUaAction(dua),
         mFunctor(func),
         mMessageAdorner(0)
      {
      }

      SendingUsageAction(DumUserAgent* dua, MessageFunctor func, MessageAdorner& adorn) :
         DumUaAction(dua),
         mMessageFunctor(func),
         mMessageAdorner(&adorn)
      {
      }      

      virtual void operator()(DumUserAgent& dua, boost::shared_ptr<Event> event)
      {
         E* specificEvent = dynamic_cast<E*>(event.get());
         
         if (specificEvent)
         {
            UsageHandleType handle = getHandleFromEvent(specificEvent);            
            if(!handle.isValid())
            {
               ErrLog(<< getName() << "::operator(), invalid handle(from DUM)");
               throw TestEndPoint::AssertException("requires a valid handle", __FILE__, __LINE__);
               }
            (*this)(handle);
         }
         else
         {
            (*this)(dua);
         }
      }

   protected:      
      virtual void operator()(DumUserAgent& dua)
      {
         
         if (getHandleFromDua().isValid())
         {
            (*this)(getHandleFromDua());
         }
         else
         {
            ErrLog(<< getName() << "::operator(), invalid handle(from DumUserAgent)");
            throw TestEndPoint::AssertException("requires a vaild handle", __FILE__, __LINE__);
         }
      }

      virtual void operator()(H handle)
      {         
         StackLog(<< getName() << "SendingUsageAction::operator(): Executing deferred call: ");
         StackLog(<< mMessageAdorner);
         
         if (mMessageAdorner)
         {
            StackLog(<< "MessageFunctor gets called");
            handle->send((*mMessageAdorner)(mMessageFunctor(*handle)));
         }
         else
         {
            StackLog(<< "Functor gets called");
            mFunctor(*handle);      
         }
      }
      

      virtual resip::Data getName() = 0;
      virtual UsageHandleType getHandleFromDua() = 0;
      virtual UsageHandleType getHandleFromEvent(E* event) = 0;

      Functor mFunctor;
      MessageFunctor mMessageFunctor;
      MessageAdorner* mMessageAdorner;
}; 

class DumUaSendingCommand : public DumUaAction{
   public:      
      typedef boost::function<resip::SharedPtr<resip::SipMessage> (void) > Functor;
      //TODO adornment functor support

      DumUaSendingCommand(DumUserAgent* dua, Functor func);
      DumUaSendingCommand(DumUserAgent* data, Functor func, MessageAdorner* adorn);
      ~DumUaSendingCommand();
      virtual void operator()(DumUserAgent& dua);
   protected:
      Functor mFunctor;
      MessageAdorner* mMessageAdorner;
};

class DumUaSendingCommandCommand : public resip::DumCommandAdapter
{
   public:
      typedef boost::function<resip::SharedPtr<resip::SipMessage> (void) > Functor;
      //TODO adornment functor support

      DumUaSendingCommandCommand(resip::DialogUsageManager& dum, Functor func, MessageAdorner* adorn);
      ~DumUaSendingCommandCommand();
      virtual void executeCommand();
      virtual EncodeStream& encodeBrief(EncodeStream& strm) const;
   protected:
      Functor mFunctor;
      MessageAdorner* mMessageAdorner;
      resip::DialogUsageManager& mDum;
};

class DumUaCommand : public DumUaAction
{
   public:
      typedef boost::function<void (void) > Functor;

      DumUaCommand(DumUserAgent* dua, Functor func);
      virtual void operator()(DumUserAgent& dua);

   protected:
      Functor mFunctor;
};

/*
// ClientPagerMessage
class ClientPagerMessageAction : public UsageAction<resip::ClientPagerMessage, ClientPagerMessageEvent, resip::ClientPagerMessageHandle>
{
   public:
      ClientPagerMessageAction(DumUserAgent* dua, Functor func)
         : UsageAction<resip::ClientPagerMessage, ClientPagerMessageEvent, resip::ClientPagerMessageHandle>(dua, func)
      {
      }


   protected:
      virtual resip::Data getName();
      virtual UsageHandleType getHandleFromDua();
      virtual UsageHandleType getHandleFromEvent(ClientPagerMessageEvent* event);
};

// ServerPagerMessage
class ServerPagerMessageAction : public SendingUsageAction<resip::ServerPagerMessage, ServerPagerMessageEvent, resip::ServerPagerMessageHandle>
{
   public:
      ServerPagerMessageAction(DumUserAgent* dua, Functor func)
         : SendingUsageAction<resip::ServerPagerMessage, ServerPagerMessageEvent, resip::ServerPagerMessageHandle>(dua, func)
      {
      }

      ServerPagerMessageAction(DumUserAgent* dua, MessageFunctor func, MessageAdorner& adorn)
         :SendingUsageAction<resip::ServerPagerMessage, ServerPagerMessageEvent, resip::ServerPagerMessageHandle>(dua, func, adorn)
      {
      }

   protected:
      virtual resip::Data getName();
      virtual UsageHandleType getHandleFromDua();
      virtual UsageHandleType getHandleFromEvent(ServerPagerMessageEvent* event);
};

// ServerOutOfDialogReq
class ServerOutOfDialogReqAction : public SendingUsageAction<resip::ServerOutOfDialogReq, ServerOutOfDialogReqEvent, resip::ServerOutOfDialogReqHandle>
{
   public:
      ServerOutOfDialogReqAction(DumUserAgent* dua, Functor func) 
         : SendingUsageAction<resip::ServerOutOfDialogReq, ServerOutOfDialogReqEvent, resip::ServerOutOfDialogReqHandle>(dua, func)
      {
      }

      ServerOutOfDialogReqAction(DumUserAgent* dua, MessageFunctor func, MessageAdorner& adorn)
         : SendingUsageAction<resip::ServerOutOfDialogReq, ServerOutOfDialogReqEvent, resip::ServerOutOfDialogReqHandle>(dua, func, adorn)
      {
      }

   protected:
      virtual resip::Data getName();
      virtual UsageHandleType getHandleFromDua();
      virtual UsageHandleType getHandleFromEvent(ServerOutOfDialogReqEvent* event);
};

// ClientOutOfDialogReq
class ClientOutOfDialogReqAction : public SendingUsageAction<resip::ClientOutOfDialogReq, ClientOutOfDialogReqEvent, resip::ClientOutOfDialogReqHandle>
{
   public:
      ClientOutOfDialogReqAction(DumUserAgent* dua, Functor func) 
         : SendingUsageAction<resip::ClientOutOfDialogReq, ClientOutOfDialogReqEvent, resip::ClientOutOfDialogReqHandle>(dua, func)
      {
      }

      ClientOutOfDialogReqAction(DumUserAgent* dua, MessageFunctor func, MessageAdorner& adorn)
         : SendingUsageAction<resip::ClientOutOfDialogReq, ClientOutOfDialogReqEvent, resip::ClientOutOfDialogReqHandle>(dua, func, adorn)
      {
      }

   protected:
      virtual resip::Data getName();
      virtual UsageHandleType getHandleFromDua();
      virtual UsageHandleType getHandleFromEvent(ClientOutOfDialogReqEvent* event);
};
*/


#undef RESIPROCATE_SUBSYSTEM

#endif

