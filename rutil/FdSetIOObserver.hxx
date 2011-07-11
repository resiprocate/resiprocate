#ifndef FdSetIOObserver_Include_Guard
#define FdSetIOObserver_Include_Guard

namespace resip
{
class FdSet;

/**
   An interface class for elements that use an FdSet to watch for IO. This is in 
   contrast to an element that uses event-driven IO.
*/
class FdSetIOObserver
{
   public:
      FdSetIOObserver(){}
      virtual ~FdSetIOObserver(){}

      /**
         Add any FDs that we are interested in watching to an fdset, with the 
         understanding that a select() call will be made immediately after.
         @param fdset The FdSet to be augmented
      */
      virtual void buildFdSet(FdSet& fdset) = 0;

      /**
         Returns the maximum timeout this object is willing to tolerate on the 
         select call that is to be made, to prevent starvation of work that is 
         not IO-based.
         @return The maximum select() timeout to be used, in milliseconds. To 
            indicate that this object does not care, return UINT_MAX. 0 
            indicates that a poll select() should be performed.
      */
      virtual unsigned int getTimeTillNextProcessMS() = 0;

      /**
         Called once select() returns; this allows this object to inspect which 
         of its FDs are ready, and perform any necessary IO with them.
         @param fdset The FdSet after the call to select().
      */
      virtual void process(FdSet& fdset) = 0;
};
}

#endif
