#include <resiprocate/SipStack.hxx>
#include <resiprocate/SipMessage.hxx>
#include <resiprocate/os/Logger.hxx>
#include <resiprocate/os/Subsystem.hxx>
#include <resiprocate/Helper.hxx>

#include <pthread.h>

class TestSS : public resip::Subsystem
{
public:
  static const TestSS RSP2;
private:
  TestSS(const resip::Data& rhs) : resip::Subsystem(resip::Data("TestSS")+rhs){};
  TestSS& operator=(const resip::Data& rhs);
}

const TestSS::RSP2("RSP2");

using namespace std;
using namespace resip;

bool g_stop_poller = false;

void *poller(void *arg)
{
  int i = 0;
  SipStack *stack1 = (SipStack*)arg;
  while (!g_stop_poller)
    {
      FdSet fdset;
      stack1->buildFdSet(fdset);
      fdset.selectMilliSeconds(100);
      stack1->process(fdset);
      SipMessage *msg = stack1->receive();
      if (msg)
        {
	  cerr << "Got a message" << endl;
	  SipMessage *resp = Helper::makeResponse(*msg,604);
	  stack1->send(*resp);
	  delete resp;
	  delete msg;
        }
      else
	{
	  ++i;
	  if (!(i%10))
	    cerr << "Nothing there " << i/10 << '\r';
	}
    }
  return NULL;
}

#define RESIPROCATE_SUBSYSTEM TestSS::RSP2

int main(int argc, char* argv[])
{
  resip::Log::initialize(Log::Cout, Log::Stack, *argv);
  SipStack stack1(argc < 2 ? false  : *argv[1] == 'm');
  //pthread_t tid;
  stack1.addTransport(UDP, 5060);
  stack1.addTransport(TCP, 5060);
  //pthread_create(&tid, NULL, poller, &stack1);
  //pthread_join(tid, &retval);
  poller((void*)&stack1);
}
