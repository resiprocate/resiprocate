#include "GagMessage.hxx"
#include "GagConduit.hxx"

GagConduit::GagConduit(SipStack &stack)
  : sipStack(&stack)
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
    case GagMessage::ERROR:
      gaimError(reinterpret_cast<GagErrorMessage *>(message));
      break;
    default:
      // XXX Yikes! What is this?
      break;
  }
}

void
GagConduit::gaimIm(GagImMessage *msg)
{
}

void
GagConduit::gaimPresence(GagPresenceMessage *msg)
{
}

void
GagConduit::gaimLogin(GagLoginMessage *msg)
{
}

void
GagConduit::gaimLogout(GagLogoutMessage *msg)
{
}

void
GagConduit::gaimAddBuddy(GagAddBuddyMessage *msg)
{
}

void
GagConduit::gaimRemoveBuddy(GagRemoveBuddyMessage *msg)
{
}

void
GagConduit::gaimError(GagErrorMessage *msg)
{
}


void
GagConduit::process()
{
  map<Uri,TuIM>::iterator tu;
  tu = tuIMList.begin();
  while (tu != tuIMList.end())
  {
    (tu->second).process();
    tu++;
  }
}

void 
GagConduit::presenseUpdate(const Uri& dest, bool open,
                           const Data& status )
{
  GagPresenceMessage presenceMessage(dest, open, status);
  presenceMessage.serialize(cout);
}

void 
GagConduit::receivedPage( const Data& msg, const Uri& from ,
                          const Data& signedBy,
                          Security::SignatureStatus sigStatus,
                          bool wasEncryped)
{
  Uri to("a@a"); // XXX
  GagImMessage imMessage(from, to, msg);
  imMessage.serialize(cout);
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

  GagErrorMessage errorMessage(error);
  errorMessage.serialize(cout);
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

  GagErrorMessage errorMessage(error);
  errorMessage.serialize(cout);
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

  GagErrorMessage errorMessage(error);
  errorMessage.serialize(cout);
}
