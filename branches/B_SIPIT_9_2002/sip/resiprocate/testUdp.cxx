
#include <sipstack/Fifo.hxx>
#include <sipstack/UdpTransport.hxx>
#include <sipstack/Logger.hxx>
#include <sipstack/Message.hxx>

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);
   Fifo<Message> fifo;
   
   DebugLog (<< "testing");
   
   UdpTransport t(5060, fifo);
   while (1)
   {
    t.process();

    //msg = sipStack.receive();
    //if ( msg )
    //{
    //cout << msg << endl;
    //}

    usleep( 50*1000); // sleep for 20 ms
  }

}
