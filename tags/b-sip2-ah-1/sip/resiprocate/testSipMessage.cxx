#include <sipstack/SipMessage.hxx>

using namespace Vocal2;

int
main()
{
   SipMessage message;
   
   //ParserCategory& foo = message[StatusLine];
   //ParserCategory& foo = message[RequestLine];
   
   ParserCategory& foo = message[To];
}
