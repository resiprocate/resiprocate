#ifndef _GAG_MESSAGE_HXX
#define _GAG_MESSAGE_HXX

#include <iostream>

#include "rutil/Data.hxx"
#include "resip/stack/Uri.hxx"

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
      GAG_ERROR              = 0x80,
      LOGIN_STATUS       = 0x81

    } command_t;

  public:
    static GagMessage *getMessage(int in_fd);
    bool isValid() {return valid;}
    virtual ostream &serialize(ostream &os) const;
    command_t getMessageType() {return messageType;}

  protected: // methods
    virtual void parse(int in_fd) = 0;
    GagMessage(){valid = true;}


    static bool parse(int , Data &);
    static bool parse(int , Uri &);
    static bool parse(int , bool &);
    static bool parse(int , int &);

    static void serialize(ostream &, const Data &);
    static void serialize(ostream &, const Uri &);
    static void serialize(ostream &, const bool &);
    static void serialize(ostream &, const int &);

  private: //methods
    static ssize_t readAll(int, char*, size_t);

  protected: // attributes
    command_t messageType;
    bool valid;
  
};

class GagImMessage : public GagMessage
{
  public:
    GagImMessage(const Uri &_from, const Uri &_to, const Data &_im) :
      from(_from), to(_to), im(_im) {messageType = IM;}

    GagImMessage(int in_fd) { messageType = IM; parse (in_fd); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(int in_fd);

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

    GagPresenceMessage(int in_fd) { messageType=PRESENCE; parse (in_fd); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(int in_fd);

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
    GagHelloMessage(int in_fd) { messageType=HELLO; parse (in_fd); }
    virtual ostream &serialize(ostream &os) const;
    virtual void parse(int in_fd);

    bool getOk() {return ok;}
  private:
    bool ok;
};

class GagLoginMessage : public GagMessage
{
  public:
    GagLoginMessage(const Uri &_aor, Data &_userid, const Data &_password,
                    const bool _register_with_service, 
                    const bool _publish_to_service):
      aor(_aor), userid(_userid), password(_password),
      register_with_service(_register_with_service),
      publish_to_service(_publish_to_service) 
      { messageType=LOGIN; }
    GagLoginMessage(int in_fd) { messageType=LOGIN; parse (in_fd); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(int in_fd);

    Uri *getAorPtr() {return &aor;}
    Data *getUseridPtr() {return &userid;}
    Data *getPasswordPtr() {return &password;}
    bool getRegisterWithService() {return register_with_service;}
    bool getPublishToService() {return publish_to_service;}
  private:
    Uri aor;
    Data userid;
    Data password;
    bool register_with_service;
    bool publish_to_service;
};

class GagLogoutMessage : public GagMessage
{
  public:
    GagLogoutMessage(const Uri &_aor) : aor(_aor) {messageType=LOGOUT;}
    GagLogoutMessage(int in_fd) {messageType=LOGOUT; parse(in_fd); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(int in_fd);

    Uri *getAorPtr() {return &aor;}
  private:
    Uri aor;
};

class GagAddBuddyMessage : public GagMessage
{
  public:
    GagAddBuddyMessage(const Uri &_us, const Uri &_them) 
      : us(_us), them(_them) { messageType=ADD_BUDDY; }
    GagAddBuddyMessage(int in_fd) { messageType=ADD_BUDDY; parse(in_fd); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(int in_fd);

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
    GagRemoveBuddyMessage(int in_fd) 
      { messageType=REMOVE_BUDDY; parse (in_fd); }

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(int in_fd);

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
    GagShutdownMessage(int in_fd) {messageType = SHUTDOWN;}
    virtual ostream &serialize(ostream &os) const {return os;}
    virtual void parse(int in_fd) {return;}
};

class GagErrorMessage : public GagMessage
{
  public:
    GagErrorMessage(const Data &_message) : message(_message)
      {messageType= GAG_ERROR;}
    GagErrorMessage(int in_fd) {messageType= GAG_ERROR; parse(in_fd);}

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(int in_fd);
  private:
    Data message;
};

class GagLoginStatusMessage : public GagMessage
{
  public:
    GagLoginStatusMessage(bool _success, int _sipCode, const Data &_message) 
      : success (_success), sipCode (_sipCode), message(_message) 
      {messageType=LOGIN_STATUS;}
    GagLoginStatusMessage(int in_fd) {messageType=LOGIN_STATUS; parse(in_fd);}

    virtual ostream &serialize(ostream &os) const;
    virtual void parse(int in_fd);

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
