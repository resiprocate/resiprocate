
class Executive
{
public:
  Executive( SipStack& stack );

  process();

private:
  SipStack& mStack;

  bool haveUdpThread;
  void processUDP();

  bool haveTCPThread;
  void processTCP();

  bool haveStateMachineThread;
  void processStateMachine();

  bool haveTimerThread;
  void processTimer();


};
