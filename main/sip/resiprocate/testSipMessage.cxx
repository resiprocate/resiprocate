#include <sipstack/SipMessage.hxx>
#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   SipMessage message;

   Data v = message.header(h_CallId).value();
   
   //StatusLine& foo = message.header(h_StatusLine);
   RequestLine& bar = message.header(h_RequestLine);
   cerr << bar.getMethod() << endl;
}
