#include "resiprocate/TuIM.hxx"
#include "GagMessage.hxx"
#include "GagConduit.hxx"

GagConduit::GagConduit(SipStack &stack, int _udpPort)
  : sipStack(&stack), udpPort(_udpPort), running(true)
{
  // Nothing interesting to do here
}

void
GagConduit::handleMessage(GagMessage *message)
{
  switch (message->getMessageType())
  {
    case GagMessage::IM:
      gaimIm(reinterpret_cast<GagImMessage *>(message));
      break;
    case GagMessage::PRESENCE:
      gaimPresence(reinterpret_cast<GagPresenceMessage *>(message));
      break;
    case GagMessage::HELLO:
      gaimHello(reinterpret_cast<GagHelloMessage *>(message));
      break;
    case GagMessage::LOGIN:
      gaimLogin(reinterpret_cast<GagLoginMessage *>(message));
      break;
    case GagMessage::LOGOUT:
      gaimLogout(reinterpret_cast<GagLogoutMessage *>(message));
      break;
    case GagMessage::ADD_BUDDY:
      gaimAddBuddy(reinterpret_cast<GagAddBuddyMessage *>(message));
      break;
    case GagMessage::REMOVE_BUDDY:
      gaimRemoveBuddy(reinterpret_cast<GagRemoveBuddyMessage *>(message));
      break;
    case GagMessage::SHUTDOWN:
      gaimShutdown(reinterpret_cast<GagShutdownMessage *>(message));
      break;
    case GagMessage::ERROR:
      gaimError(reinterpret_cast<GagErrorMessage *>(message));
      break;
    case GagMessage::LOGIN_STATUS:
      gaimLoginStatus(reinterpret_cast<GagLoginStatusMessage *>(message));
      break;
    default:
      // XXX Yikes! What is this?
      break;
  }
}


GagConduit::~GagConduit()
{
  removeAllUsers();
}

void
GagConduit::removeAllUsers()
{
  // Shut down all logged in users
  map<Uri,TuIM *>::iterator tu;
  tu = tuIM.begin();
  while (tu != tuIM.end())
  {
    delete(tu->second);
    tuIM.erase(tu);
    tu++;
  }
}

map<Uri,TuIM *>::iterator
GagConduit::getTu(Uri &aor)
{
  map<Uri,TuIM *>::iterator tu = tuIM.find(aor);
  if (tu == tuIM.end())
  {
    Data error;
    error = "You are not logged in as ";
    error += aor.getAor();
    GagErrorMessage(error).serialize(cout);
  }
  return tu;
}

void
GagConduit::gaimIm(GagImMessage *msg)
{
  Uri *from = msg->getFromPtr();
  Uri *to = msg->getToPtr();
  Data *im = msg->getImPtr();

  map<Uri,TuIM *>::iterator tu = getTu(*from);
  if (tu == tuIM.end()) return;

  tu->second->sendPage(*im, *to, false, to->getAor());
}

void
GagConduit::gaimPresence(GagPresenceMessage *msg)
{
  Uri *aor = msg->getAorPtr();
  bool online = msg->getAvailable();
  Data *status = msg->getStatusPtr();

  map<Uri,TuIM *>::iterator tu = getTu(*aor);
  if (tu == tuIM.end()) return;
  
  tu->second->setMyPresence(online, *status);
}

void
GagConduit::gaimHello(GagHelloMessage *msg)
{
  // We don't need to do anything when we get a hello.
  // Not yet, at least.
}

void
GagConduit::gaimLogin(GagLoginMessage *msg)
{
  Uri *aor = msg->getAorPtr();
  Data *userid = msg->getUseridPtr();
  Data *password = msg->getPasswordPtr();
  TuIM *newTu;


  map<Uri,TuIM *>::iterator tu = tuIM.find(*aor);
  if (tu != tuIM.end())
  {
    Data error;
    error = "You are already logged in as ";
    error += aor->getAor();
    GagErrorMessage(error).serialize(cout);
  }

  // Figure out what out contact is
  Uri contact;
  contact.port() = udpPort;
  contact.host() = sipStack->getHostAddress();
  contact.user() = aor->user();

  newTu = new TuIM(sipStack, *aor, contact, this);
  // XXX Check for null here
  newTu->setUAName(Data("gag/0.0.0 (gaim)"));
  newTu->registerAor(*aor, *password);

  tuIM[*aor] = newTu;
}

void
GagConduit::gaimLogout(GagLogoutMessage *msg)
{
  Uri *aor = msg->getAorPtr();
  map<Uri,TuIM *>::iterator tu = getTu(*aor);
  if (tu == tuIM.end()) return;
  
  delete(tu->second);
  tuIM.erase(tu);
}

void
GagConduit::gaimAddBuddy(GagAddBuddyMessage *msg)
{
  Uri *us = msg->getUsPtr();
  Uri *them = msg->getThemPtr();

  map<Uri,TuIM *>::iterator tu = getTu(*us);
  if (tu == tuIM.end()) return;

  tu->second->addBuddy(*them, Data::Empty);
}

void
GagConduit::gaimRemoveBuddy(GagRemoveBuddyMessage *msg)
{
  Uri *us = msg->getUsPtr();
  Uri *them = msg->getThemPtr();

  map<Uri,TuIM *>::iterator tu = getTu(*us);
  if (tu == tuIM.end()) return;

  tu->second->removeBuddy(*them);
}

void
GagConduit::gaimShutdown(GagShutdownMessage *msg)
{
  running = false;
  removeAllUsers();
}

void
GagConduit::gaimError(GagErrorMessage *msg)
{
  // XXX This should *never* be called..
}

void
GagConduit::gaimLoginStatus(GagLoginStatusMessage *msg)
{
  // XXX This should *never* be called..
}

void
GagConduit::process()
{
  map<Uri,TuIM *>::iterator tu;
  tu = tuIM.begin();
  while (tu != tuIM.end())
  {
    (tu->second)->process();
    tu++;
  }
}

void 
GagConduit::presenceUpdate(const Uri& dest, bool open,
                           const Data& status )
{
  GagPresenceMessage(dest, open, status).serialize(cout);
}

void 
GagConduit::receivedPage( const Data& msg, const Uri& from ,
                          const Data& signedBy,
                          Security::SignatureStatus sigStatus,
                          bool wasEncryped)
{
  Uri to("a@a"); // XXX
  GagImMessage (from, to, msg).serialize(cout);
}

void 
GagConduit::sendPageFailed( const Uri& dest,int respNumber )
{
  Data error;
  error = "Could not send IM to ";
  error += dest.getAor();
  error = " (";
  error += respNumber;
  error = ")";

  GagErrorMessage (error).serialize(cout);
}

void 
GagConduit::registrationFailed(const resip::Uri& uri, int respNumber)
{
  // XXX Should be something other than a generic error
  Data error;
  error = "Could not register as ";
  error += uri.getAor();
  error = " (";
  error += respNumber;
  error = ")";

  GagErrorMessage (error).serialize(cout);
}

void 
GagConduit::registrationWorked(const Uri& dest )
{
  // XXX
}

void 
GagConduit::receivePageFailed(const Uri& sender)
{
  Data error;
  error = "Could not get IM from ";
  error += sender.getAor();

  GagErrorMessage (error).serialize(cout);
}
