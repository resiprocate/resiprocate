#include <sipstack/SipMessage.hxx>

using namespace Vocal2;

int
main()
{
   SipMessage message;
   
   //StatusLineComponent& foo = message[StatusLine];
   RequestLineComponent& bar = message[RequestLine];
   cerr << bar.getMethod() << endl;
   
   //ParserCategory& foo = message[To];
}
