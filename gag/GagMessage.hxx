#ifndef _GAG_MESSAGE_HXX
#define _GAG_MESSAGE_HXX

#include <iostream>

#include "resiprocate/os/Data.hxx"
#include "resiprocate/Uri.hxx"

using namespace resip;
using namespace std;

class GagMessage
{
  public:
    typedef enum
    {
      // GAIM <-> GAG
      IM                 = 0x00,
      PRESENCE           = 0x01,
      HELLO              = 0x02,
  
      // GAIM --> GAG
      LOGIN              = 0x40,
      LOGOUT             = 0x41,
      ADD_BUDDY          = 0x42,
      REMOVE_BUDDY       = 0x43,
      SHUTDOWN           = 0x44,

      // GAG --> GAIM
      ERROR              = 0x80,
      LOGIN_STATUS       = 0x81

    } command_t;

  public:
    static GagMessage *getMessage(istream &is);
    bool isValid() {return valid;}
    virtual ostream &serialize(ostream &os) const;
    command_t getMessageType() {return messageType;}

  protected: // methods
    virtual void parse(istream &is) = 0;
    GagMessage(){valid = true;}


    static bool parse(istream &, Data &);
    static bool parse(istream &, Uri &);
    static bool parse(istream &, bool &);

    static void serialize(ostream &, const Data &);
    static void serialize(ostream &, const Uri &);
    static void serialize(ostream &, const bool &);

  protected: // attributes
    command_t messageType;
    bool valid;
};

class GagImMessage : public GagMessage
{
  public:
    GagImMessage(const Uri &_from, const Uri &_to, const Data &_im) :
      from(_from), to(_to), im(_im) {messageType = IM;}

    GagImMessage(istream &is) { messageType = IM; parse (is); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(istream &is);

    Uri *getFromPtr() {return &from;}
    Uri *getToPtr() {return &to;}
    Data *getImPtr() {return &im;}
  private:
    Uri from;
    Uri to;
    Data im;
};

class GagPresenceMessage : public GagMessage
{
  public:
    GagPresenceMessage(const Uri &_aor, bool _available, const Data &_status):
      aor(_aor), available(_available), status(_status)
      { messageType=PRESENCE ;}

    GagPresenceMessage(istream &is) { messageType=PRESENCE; parse (is); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(istream &is);

    Uri *getAorPtr() {return &aor;}
    bool getAvailable() {return available;}
    Data *getStatusPtr() {return &status;}
  private:
    Uri aor;
    bool available;
    Data status;
};

class GagHelloMessage : public GagMessage
{
  public:
    GagHelloMessage(bool _ok) : ok(_ok) { messageType=HELLO; }
    GagHelloMessage(istream &is) { messageType=HELLO; parse (is); }
    virtual ostream &serialize(ostream &os) const;
    virtual void parse(istream &is);

    bool getOk() {return ok;}
  private:
    bool ok;
};

class GagLoginMessage : public GagMessage
{
  public:
    GagLoginMessage(const Uri &_aor, Data &_userid, const Data &_password):
      aor(_aor), userid(_userid), password(_password)
      { messageType=LOGIN; }
    GagLoginMessage(istream &is) { messageType=LOGIN; parse (is); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(istream &is);

    Uri *getAorPtr() {return &aor;}
    Data *getUseridPtr() {return &userid;}
    Data *getPasswordPtr() {return &password;}
  private:
    Uri aor;
    Data userid;
    Data password;
};

class GagLogoutMessage : public GagMessage
{
  public:
    GagLogoutMessage(const Uri &_aor) : aor(_aor) {messageType=LOGOUT;}
    GagLogoutMessage(istream &is) {messageType=LOGOUT; parse(is); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(istream &is);

    Uri *getAorPtr() {return &aor;}
  private:
    Uri aor;
};

class GagAddBuddyMessage : public GagMessage
{
  public:
    GagAddBuddyMessage(const Uri &_us, const Uri &_them) 
      : us(_us), them(_them) { messageType=ADD_BUDDY; }
    GagAddBuddyMessage(istream &is) { messageType=ADD_BUDDY; parse(is); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(istream &is);

    Uri *getUsPtr() {return &us;}
    Uri *getThemPtr() {return &them;}
  private:
    Uri us;
    Uri them;
};

class GagRemoveBuddyMessage : public GagMessage
{
  public:
    GagRemoveBuddyMessage(const Uri &_us, const Uri &_them) 
      : us(_us), them(_them) { messageType=REMOVE_BUDDY; }
    GagRemoveBuddyMessage(istream &is) 
      { messageType=REMOVE_BUDDY; parse (is); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(istream &is);

    Uri *getUsPtr() {return &us;}
    Uri *getThemPtr() {return &them;}
  private:
    Uri us;
    Uri them;
};

class GagShutdownMessage : public GagMessage
{
  public:
    GagShutdownMessage() {messageType = SHUTDOWN;}
    GagShutdownMessage(istream &is) {messageType = SHUTDOWN;}
    virtual ostream &serialize(ostream &os) const {return os;}
    virtual void parse(istream &is) {return;}
};

class GagErrorMessage : public GagMessage
{
  public:
    GagErrorMessage(const Data &_message) : message(_message)
      {messageType=ERROR;}
    GagErrorMessage(istream &is) {messageType=ERROR; parse(is);}

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(istream &is);
  private:
    Data message;
};

class GagLoginStatusMessage : public GagMessage
{
  public:
    GagLoginStatusMessage(bool _success, int _sipCode, const Data &_message) 
      : success (_success), sipCode (_sipCode), message(_message) 
      {messageType=LOGIN_STATUS;}
    GagLoginStatusMessage(istream &is) {messageType=LOGIN_STATUS; parse(is);}

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(istream &is);

    bool succeeded() {return success;}
    int getSipCode() {return success;}
    Data *getMessagePtr() {return &message;}
  private:
    bool success;
    int sipCode;
    Data message;
};

inline
ostream &
operator<< (ostream &os, const GagMessage &msg)
{
  return msg.serialize(os);
}

#endif
