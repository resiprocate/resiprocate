#ifndef _GAG_CONDUIT_HXX
#define _GAG_CONDUIT_HXX

#include <list>

#include "resiprocate/TuIM.hxx"
#include "resiprocate/SipStack.hxx"
#include "GagMessage.hxx"

class GagConduit: public TuIM::Callback
{
  public:
    GagConduit(SipStack &stack);

    void handleMessage(GagMessage *);
    void process();

    // Methods to handle commands from GAIM
    void gaimIm(GagImMessage *);
    void gaimPresence(GagPresenceMessage *);
    void gaimLogin(GagLoginMessage *);
    void gaimLogout(GagLogoutMessage *);
    void gaimAddBuddy(GagAddBuddyMessage *);
    void gaimRemoveBuddy(GagRemoveBuddyMessage *);
    void gaimError(GagErrorMessage *);

    // Callback Methods
    virtual void presenseUpdate(const Uri& dest, bool open, 
                                const Data& status );
    virtual void receivedPage( const Data& msg, const Uri& from ,
                               const Data& signedBy,
                               Security::SignatureStatus sigStatus,
                               bool wasEncryped);
    virtual void sendPageFailed( const Uri& dest,int respNumber );
    virtual void registrationFailed(const resip::Uri&, int respNumber);
    virtual void registrationWorked(const Uri& dest );
    virtual void receivePageFailed(const Uri& sender);

  private:
    // Here are all of our TUs
    list<TuIM> tuIMList;
    SipStack *sipStack;
};
#endif
