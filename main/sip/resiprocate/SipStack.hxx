
class SipStack
{
public:
    void send( Messagee msg , String dest=default );

    Message* receive();

    void process();

    /// returns time in milliseconds when process next needs to be called 
    int getTimeTillNextProcess(); 

    enum ThreadFunction { Timer, UDP, StateMachine, TCP, };

    void runThread( ThreadFunction );


private:

    Executive mExecutive;

    TransactionMap mTransactionMap;

    TransportDirectory mTransportDirector;

    Fifo mRxFifo;

    Fifo mTUFifo;

    Fifo mStateMacFifo;

    TimerWheel mTimers;

};

