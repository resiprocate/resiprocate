#ifndef ProducerFifoBuffer_Include_Guard
#define ProducerFifoBuffer_Include_Guard

#include "rutil/Fifo.hxx"

namespace resip
{

/**
   Class for buffering messages placed in a Fifo, to be used by a single 
   producer. By adding messages in bulk, lock contention is reduced.
   @todo Refactor so we can use this with AbstractFifo<T>? This is presently not 
      the case because AbstractFifo does not expose its API publicly.
*/
template<typename T>
class ProducerFifoBuffer
{
   public:
      ProducerFifoBuffer(Fifo<T>& fifo,
                           size_t bufferSize) :
         mFifo(fifo),
         mBufferSize(bufferSize)
      {}

      ~ProducerFifoBuffer()
      {
         flush();
      }

      void add(T* msg)
      {
         mBuffer.push_back(msg);
         if(mBuffer.size()>=mBufferSize)
         {
            flush();
         }
      }

      void flush()
      {
         mFifo.addMultiple(mBuffer);
      }

      inline size_t getBufferSize() const
      {return mBufferSize;}

      inline void setBufferSize(size_t pBufferSize)
      {
         mBufferSize = pBufferSize;
         if(mBuffer.size()>=mBufferSize)
         {
            flush();
         }
      }

      inline const Fifo<T>& getFifo() const {return mFifo;} 
      inline Fifo<T>& getFifo() {return mFifo;} 

   protected:
      Fifo<T>& mFifo;
      // The type used by AbstractFifo<T>::addMultiple()
      typename Fifo<T>::Messages mBuffer;
      size_t mBufferSize;
};
}
#endif
