// ==========================================================================================================
// InternalRejectIncomingMessage.cxx                                                     2006 @ TelTel
// ==========================================================================================================
// Ends active invite session for thread synchronization (onTerminated() gets called in same thread as
// other virtual handlers).
// ==========================================================================================================
#include "InternalRejectIncomingMessage.hxx"

#include "resiprocate/dum/ServerInviteSession.hxx"

using namespace resip;

InternalRejectIncomingMessage::InternalRejectIncomingMessage(ServerInviteSessionHandle& h,
                                                             int statusCode,
                                                             WarningCategory* warning)
: mIncomingSession(h)
 ,mStatusCode(statusCode)
{
   if (warning)
   {
      mWarning.reset(new WarningCategory(*warning));
   }
}

std::ostream& InternalRejectIncomingMessage::encode(std::ostream& strm) const
{
   return encodeBrief(strm);
}

std::ostream& InternalRejectIncomingMessage::encodeBrief(std::ostream& strm) const
{
   return strm << "InternalRejectIncomingMessage";
}

