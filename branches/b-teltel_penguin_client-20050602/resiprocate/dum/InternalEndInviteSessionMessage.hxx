// ==========================================================================================================
// InternalEndInviteSessionMessage.hxx                                                   2006 @ TelTel
// ==========================================================================================================
// Ends active invite session for thread synchronization (onTerminated() gets called in same thread as
// other virtual handlers).
// ==========================================================================================================
#ifndef RESIP_InternalEndInviteSessionMessage_hxx
#define RESIP_InternalEndInviteSessionMessage_hxx

#include <cassert>
#include <iosfwd>

#include "resiprocate/Message.hxx"
#include "resiprocate/dum/Win32ExportDum.hxx"
#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/dum/InviteSession.hxx"

namespace resip
{

   class DUM_API InternalEndInviteSessionMessage : public Message
   {
   public:
      RESIP_HeapCount(InternalEndInviteSessionMessage);
      InternalEndInviteSessionMessage(InviteSessionHandle& h) : mInviteSession(h) {/*Empty*/}

      virtual Message* clone() const {assert(false); return NULL;}
      virtual std::ostream& encode(std::ostream& strm) const { return encodeBrief(strm); }
      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalEndInviteSessionMessage"; }

      InviteSessionHandle  mInviteSession;   // valid (don't care if connected or not).
   };

}

#endif // RESIP_InternalEndInviteSessionMessage_hxx

