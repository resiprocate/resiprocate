#if !defined(EXECUTIVE_HXX)
#define EXECUTIVE_HXX

#include <util/Socket.hxx>
#include <sys/types.h>
#include <sys/select.h> // posix

namespace Vocal2
{

class SipStack;

class Executive
{
   public:
      Executive( SipStack& stack );

      void process(fd_set* fdSet);

	// build the FD set to use in a select to find out when process bust be called again
	void buildFdSet( fd_set* fdSet, int* fdSetSize );  

	/// returns time in milliseconds when process next needs to be called 
      int getTimeTillNextProcess(); 

   private:
      SipStack& mStack;

      bool processTransports(fd_set* fdSet); // return true if more work to do

      bool processStateMachine();// return true if more work to do

      bool processTimer();// return true if more work to do
};

}

#endif
