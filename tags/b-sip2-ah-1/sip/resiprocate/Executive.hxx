#if !defined(EXECUTIVE_HXX)
#define EXECUTIVE_HXX

namespace Vocal2
{

  class SipStack;

class Executive
{
public:
  Executive( SipStack& stack );

  void process();

private:
  SipStack& mStack;

  //bool haveUdpThread;
  bool processTransports(); // return true if more work to do

  //bool haveTCPThread;
  //void processTCP();

  ///bool haveStateMachineThread;
  bool processStateMachine();// return true if more work to do

  //bool haveTimerThread;
  bool processTimer();// return true if more work to do


};

}

#endif
