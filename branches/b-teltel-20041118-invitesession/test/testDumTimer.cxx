#include "resiprocate/SipStack.hxx"

#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/os/Data.hxx"

using namespace resip;


// class testDum : public BaseUsage
// {
//    public:
// };

int 
main (int argc, char** argv)
{
    using namespace std;

    bool done = false;
    SipStack stack;
    SipMessage msg;
    msg.header(h_CallId).value() = Data("test-call-id-alice%40example.com");
//   stackA.addTransport(UDP, 5060);

    DialogUsageManager dum(stack);
    
    cout << "Created DUM, schduling timer for testDum Object" << endl;

    // Make timer.

    while ( (!done) )
    {
       FdSet fdset;
       // Should these be buildFdSet on the DUM?
       int err = fdset.selectMilliSeconds(100);
       assert ( err != -1 );
       dum.process(fdset);
    }   

   // How do I turn these things off? For now, we just blow
   // out with all the wheels turning...

}
