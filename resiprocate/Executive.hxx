
class Executive
{
public:
  Executive( SipStack& stack );

  process();

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
