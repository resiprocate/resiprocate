#include <cassert>
#include "resiprocate/os/AbstractFifo.hxx"

using namespace resip;

AbstractFifo::AbstractFifo(unsigned int maxSize) 
   : mSize(0),
     mMaxSize(maxSize)
{
}

AbstractFifo::~AbstractFifo()
{
}

void*
AbstractFifo ::getNext()
{
   Lock lock(mMutex); (void)lock;

   // Wait while there are messages available.
   //
   while (mFifo.empty())
   {
      mCondition.wait(&mMutex);
   }

   // Return the first message on the fifo.
   //
   void* firstMessage = mFifo.front();
   mFifo.pop_front();
   assert(mSize != 0);
   mSize--;
   return firstMessage;
}

bool
AbstractFifo::empty() const
{
   return mSize == 0;
}

unsigned int
AbstractFifo ::size() const
{
   Lock lock(mMutex); (void)lock;
   return mSize; 
}

bool
AbstractFifo::messageAvailable() const
{
   Lock lock(mMutex); (void)lock;
   assert(mSize != NoSize);
   return !mFifo.empty();
}
