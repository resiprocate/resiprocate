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
      break;
    case GagMessage::PRESENCE:
      break;
    case GagMessage::LOGIN:
      break;
    case GagMessage::LOGOUT:
      break;
    case GagMessage::ADD_BUDDY:
      break;
    case GagMessage::REMOVE_BUDDY:
      break;
    case GagMessage::ERROR:
      break;
  }
}

void
GagConduit::process()
{
  list<TuIM>::iterator tu;
  tu = tuIMList.begin();
  while (tu != tuIMList.end())
  {
    tu->process();
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
