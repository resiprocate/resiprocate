#ifndef ConsumerFifoBuffer_Include_Guard
#define ConsumerFifoBuffer_Include_Guard

namespace resip
{
template<typename T>
class ConsumerFifoBuffer
{
   public:
      ConsumerFifoBuffer(Fifo<T>& fifo) :
         mFifo(fifo)
      {}

      ~ConsumerFifoBuffer()
      {}

      
};
}

#endif
