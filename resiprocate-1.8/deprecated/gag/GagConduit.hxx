#ifndef _GAG_CONDUIT_HXX
#define _GAG_CONDUIT_HXX

#include <map>

#include "resip/stack/TuIM.hxx"
#include "resip/stack/SipStack.hxx"
#include "GagMessage.hxx"

class GagConduit: public TuIM::Callback
{
  public:
    GagConduit(SipStack &stack, int udpPort);
    ~GagConduit();

    void handleMessage(GagMessage *);
    void process();
    bool isRunning() { return running; }
    void removeAllUsers();

    // Methods to handle commands from GAIM
    void gaimIm(GagImMessage *);
    void gaimPresence(GagPresenceMessage *);
    void gaimHello(GagHelloMessage *);
    void gaimLogin(GagLoginMessage *);
    void gaimLogout(GagLogoutMessage *);
    void gaimAddBuddy(GagAddBuddyMessage *);
    void gaimRemoveBuddy(GagRemoveBuddyMessage *);
    void gaimShutdown(GagShutdownMessage *);
    void gaimError(GagErrorMessage *);
    void gaimLoginStatus(GagLoginStatusMessage *);

    // Callback Methods
    virtual void presenceUpdate(const Uri& dest, bool open, 
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
    map<Uri,TuIM *>::iterator getTu(Uri &aor);

  private:
    // Here are all of our TUs
    map<Uri,TuIM *> tuIM;
    SipStack *sipStack;
    int udpPort;
    bool running;
};
#endif
