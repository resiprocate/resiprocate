#include <sip2/sipstack/Fifo.hxx>
#include <sip2/sipstack/UdpTransport.hxx>

using namespace Vocal2;

int
main(int argc, char* argv[])
{
   Fifo<Message> fifo;
   
   
   UdpTransport t(5060, fifo);
   t.run();
}
