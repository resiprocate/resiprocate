#include <resiprocate/SipStack.hxx>
#include <resiprocate/SipMessage.hxx>

#include <pthread.h>

using namespace std;
using namespace resip;

bool g_stop_poller = false;

void *poller(void *arg)
{
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
            delete msg;
        }
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    SipStack stack1;
    pthread_t tid;
    void *retval;
    stack1.addTransport(UDP, 5060);
    stack1.addTransport(TCP, 5060);
    //    pthread_create(&tid, NULL, poller, &stack1);
    //    pthread_join(tid, &retval);
    poller((void*)&stack1);
}
