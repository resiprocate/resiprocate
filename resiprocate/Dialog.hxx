#if !defined(DIALOG_HXX)
#define DIALOG_HXX

#include <iostream>
#include <sipstack/SipMessage.hxx>

namespace Vocal2
{

class Dialog
{
   public:
      // pass in a contact for this location e.g. "sip:local@domain:5060"
      Dialog(Url& localContact);
      
      // This happens when a dialog gets created on a UAS when a 
      // provisional (1xx) response or 2xx is sent back by the UAS
      void createDialogAsUAS(SipMessage& request, SipMessage& response);
      
      // This happens when a dialog gets created on a UAC when 
      // a UAC receives a response that creates a dialog
      void createDialogAsUAC(SipMessage& request, SipMessage& response);

      // Called when a 2xx response is received in an existing dialog
      // Replace the _remoteTarget with uri from Contact header in response
      void targetRefreshResponse(SipMessage& response);

      // Called when a request is received in an existing dialog
      // return status code of response to generate - 0 if ok
      int targetRefreshRequest(SipMessage& request);

      const Data& dialogId() const { return mDialogId; }
      const Data& getLocalTag() const { return mLocalTag; }
      const Url& getRemoteTarget() const { return mRemoteTarget; }

      // For creating requests within a dialog
      SipMessage makeInvite();
      SipMessage makeBye();
      SipMessage makeRefer(Url& referTo);
      SipMessage makeNotify();
      SipMessage makeOptions();
      SipMessage makeAck(SipMessage& request);
      SipMessage makeCancel(SipMessage& request);
      
      // resets to an empty dialog with no state
      void clear();
      
   private:
      void setRequestDefaults(SipMessage& request);
      void incrementCSeq(SipMessage& request);
      void copyCSeq(SipMessage& request);

      Via mVia;          // for this UA
      Url mContact;  // for this UA

      // Dialog State
      bool mCreated;
      Urls mRouteSet;
      Url mRemoteTarget;
      unsigned long mRemoteSequence;
      bool mRemoteEmpty;
      unsigned long mLocalSequence;
      bool mLocalEmpty;
      CallId mCallId;
      Data mLocalTag;
      Data mRemoteTag;
      Url mRemoteUri;
      Url mLocalUri;
      Data mDialogId;

      friend std::ostream& operator<<(std::ostream& strm, Dialog& d);
};

std::ostream&
operator<<(std::ostream& strm, Dialog& d);
 
} // namespace Cathay

#endif
