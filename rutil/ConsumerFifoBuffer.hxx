#ifndef ConsumerFifoBuffer_Include_Guard
#define ConsumerFifoBuffer_Include_Guard

#include "rutil/Fifo.hxx"

namespace resip
{
template<typename T>
class ConsumerFifoBuffer
{
   public:
      ConsumerFifoBuffer(Fifo<T>& fifo,
                           unsigned int bufferSize=8) :
         mFifo(fifo),
         mBufferSize(bufferSize)
      {}

      ~ConsumerFifoBuffer()
      {}

      T* getNext(int ms=0)
      {
         if(mBuffer.empty())
         {
            mFifo.getMultiple(ms, mBuffer, mBufferSize);
         }

         if(mBuffer.empty())
         {
            return 0;
         }

         T* next(mBuffer.front());
         mBuffer.pop_front();
         return next;
      }

      bool messageAvailable() const
      {
         if(mBuffer.empty())
         {
            return mFifo.messageAvailable();
         }
         return true;
      }

      unsigned int size() const
      {
         return mBuffer.size() + mFifo.size();
      }

   private:
      Fifo<T>& mFifo;
      typename Fifo<T>::Messages mBuffer;
      unsigned int mBufferSize;
};
}

#endif
