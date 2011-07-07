#include "tfm/tfdum/DumUaAction.hxx"
#include "tfm/tfdum/DumUserAgent.hxx"
//#include "tfm/tfdum/DumEvent.hxx"
//#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

void
DumUaAction::operator()(boost::shared_ptr<Event> event)
{
   //!dcm! having resip::SharedPtr is annoying now..two dynamic_pointer_cast templates
   boost::shared_ptr<DumEvent> dumEvent 
      = boost::dynamic_pointer_cast<DumEvent, Event>(event);        
   return (*this)(*mUa, dumEvent);
}

void
DumUaAction::operator()()
{
   return (*this)(*mUa);
}

void 
DumUaAction::operator()(DumUserAgent& tua, boost::shared_ptr<DumEvent> event)
{
   return (*this)(*mUa);
}

Start::Start(DumUserAgent& dua)
   : DumUaAction(&dua)
{
}

void 
Start::operator()(DumUserAgent& dua)
{
//   dua.run();
}

Data 
Start::toString() const 
{
   return mUa->getName() + ".start()";
}

Data 
Shutdown::toString() const
{
   return mUa->getName() + ".shutdown()";
}

Shutdown::Shutdown(DumUserAgent& dua)
   : DumUaAction(&dua)
{
}

void 
Shutdown::operator()(DumUserAgent& dua)
{
//   dua.stop();   
}

DumUaSendingCommand::DumUaSendingCommand(DumUserAgent* dua, Functor func) :
   DumUaAction(dua),
   mFunctor(func),
   mMessageAdorner(0)
{
}

DumUaSendingCommand::DumUaSendingCommand(DumUserAgent* dua, Functor func, MessageAdorner* adorner) :
   DumUaAction(dua),
   mFunctor(func),
   mMessageAdorner(adorner)
{
}

DumUaSendingCommand::~DumUaSendingCommand()
{
   delete mMessageAdorner;
}

void 
DumUaSendingCommand::operator()(DumUserAgent& dua)
{
   dua.getDum().post(new DumUaSendingCommandCommand(dua.getDum(), mFunctor, mMessageAdorner));
   mMessageAdorner=0;
}

DumUaSendingCommandCommand::DumUaSendingCommandCommand(resip::DialogUsageManager& dum, Functor func, MessageAdorner* adorn) :
   mFunctor(func),
   mMessageAdorner(adorn),
   mDum(dum)
{}

DumUaSendingCommandCommand::~DumUaSendingCommandCommand()
{
   delete mMessageAdorner;
}

EncodeStream& 
DumUaSendingCommandCommand::encodeBrief(EncodeStream& strm) const
{
   return strm << "DumUaSendingCommandCommand" << std::endl;
}

void
DumUaSendingCommandCommand::executeCommand()
{
   StackLog(<< "DumUaSendingCommand::operator(): Executing deferred call: ");
   SharedPtr<SipMessage> msg = mFunctor();
   if (mMessageAdorner)
   {
      mDum.send((*mMessageAdorner)(msg));
   }
   else
   {
      mDum.send(msg);
   }
}

DumUaCommand::DumUaCommand(DumUserAgent* dua, Functor func) :
   DumUaAction(dua),
   mFunctor(func)
{
}

void
DumUaCommand::operator()(DumUserAgent& dua)
{
   StackLog(<< "DumUaCommand::operator(): Executing deferred call.");
   mFunctor();
}

/*
Data
ClientPagerMessageAction::getName()
{
  return "ClientPagerMessageAction";
}

ClientPagerMessageAction::UsageHandleType
ClientPagerMessageAction::getHandleFromDua()
{
  return mUa->mClientPagerMessage;
}

ClientPagerMessageAction::UsageHandleType 
ClientPagerMessageAction::getHandleFromEvent(ClientPagerMessageEvent* event)
{
   return event->getUsageHandle();
}
*/

/*
Data
ServerPagerMessageAction::getName()
{
  return "ServerPagerMessageAction";
}

ServerPagerMessageAction::UsageHandleType
ServerPagerMessageAction::getHandleFromDua()
{
  return mUa->mServerPagerMessage;
}

ServerPagerMessageAction::UsageHandleType 
ServerPagerMessageAction::getHandleFromEvent(ServerPagerMessageEvent* event)
{
   return event->getUsageHandle();
}
*/

/*
Data
ServerOutOfDialogReqAction::getName()
{
  return "ServerOutOfDialogReqAction";
}

ServerOutOfDialogReqAction::UsageHandleType
ServerOutOfDialogReqAction::getHandleFromDua()
{
  return mUa->mServerOutOfDialogReq;
}

ServerOutOfDialogReqAction::UsageHandleType
ServerOutOfDialogReqAction::getHandleFromEvent(ServerOutOfDialogReqEvent* event)
{
  return event->getUsageHandle();
}

Data
ClientOutOfDialogReqAction::getName()
{
  return "ClientOutOfDialogReqAction";
}

ClientOutOfDialogReqAction::UsageHandleType
ClientOutOfDialogReqAction::getHandleFromDua()
{
  return mUa->mClientOutOfDialogReq;
}

ClientOutOfDialogReqAction::UsageHandleType
ClientOutOfDialogReqAction::getHandleFromEvent(ClientOutOfDialogReqEvent* event)
{
  return event->getUsageHandle();
}
*/
