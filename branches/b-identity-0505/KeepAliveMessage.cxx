#include "resiprocate/KeepAliveMessage.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

using namespace resip;

KeepAliveMessage::KeepAliveMessage()
{
}

KeepAliveMessage::KeepAliveMessage(const KeepAliveMessage& message)
   : SipMessage(message)
{
   header(h_RequestLine).method() = OPTIONS;
   Via via;
   header(h_Vias).push_back(via);
}

KeepAliveMessage::~KeepAliveMessage()
{   
}


KeepAliveMessage&
KeepAliveMessage::operator=(const KeepAliveMessage& rhs)
{
   if (this != &rhs)
   {
      SipMessage::operator=(rhs); 
   }
   return *this;
}

Message*
KeepAliveMessage::clone() const
{
   return new KeepAliveMessage(*this);
}

std::ostream&
KeepAliveMessage::encode(std::ostream& str) const
{
   str << Symbols::CRLF << Symbols::CRLF;
   return str;
}
