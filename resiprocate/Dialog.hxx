#if !defined(DIALOG_HXX)
#define DIALOG_HXX

#include <sipstack/SipMessage.hxx>

namespace Vocal2
{

class Dialog
{
   public:
      // pass in a contact for this location e.g. "sip:local@domain:5060"
      Dialog(const Data& localContact);

      // This happens when a dialog gets created on a UAS when a 
      // provisional (1xx) response or 2xx is sent back by the UAS
      void createDialogAsUAS(const SipMessage& request,
                             const SipMessage& response);
      
      // This happens when a dialog gets created on a UAC when 
      // a UAC receives a response that creates a dialog
      void createDialogAsUAC(const SipMessage& request,
                             const SipMessage& response);

      // Called when a 2xx response is received in an existing dialog
      // Replace the _remoteTarget with uri from Contact header in response
      void targetRefresh(const SipMessage& response);

      // Called when a request is received in an existing dialog
      // return status code of response to generate - 0 if ok
      int targetRefresh(const SipMessage& request);

      const Data& dialogId() const { return _dialogId; }
      const Data& getLocalTag() const { return _localTag; }
      const NameAddr& getRemoteTarget() const { return _remoteTarget; }

      // For creating requests within a dialog
      SipMessage makeInvite();
      SipMessage makeBye();
      SipMessage makeRefer(const Sptr<Vocal::BaseUrl>& referTo);
      SipMessage makeNotify();
      SipMessage makeOptions();
      SipMessage makeAck(const Sptr<Vocal::SipCommand>& request);
      SipMessage makeCancel(const Sptr<Vocal::SipCommand>& request);

      // resets to an empty dialog with no state
      void clear();
      
   private:
      void setRequestDefaults(SipMessage& request);
      void incrementCSeq(SipMessage& request);
      void copyCSeq(SipMessage& request);

      Via _via;          // for this UA
      Contact _contactUrl;  // for this UA

      // Dialog State
      bool _created;
      NameAddrs _routeSet;
      NameAddr _remoteTarget;
      unsigned long _remoteSequence;
      bool _remoteEmpty;
      unsigned long _localSequence;
      bool _localEmpty;
      CallId _callId;
      Data _localTag;
      Data _remoteTag;
      NameAddr _remoteUri;
      NameAddr _localUri;

      Data _dialogId;

      friend ostream& operator<<(ostream& strm, Dialog& d);
};

ostream&
operator<<(ostream& strm, Dialog& d);
 
} // namespace Cathay

#endif
