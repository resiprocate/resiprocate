#if !defined(RESIP_NOTIFIER_FIFO_HXX)
#define RESIP_NOTIFIER_FIFO_HXX

#include "resiprocate/os/Fifo.hxx"

/** Infrastructure common to VOCAL.
 */
namespace resip
{

/** First in first out list interface, with the added functionality of 
 *  being able to handle timed entries.
 */
template < class Msg >
class NotifierFifo : public Fifo<Msg>, public ProcessNotifier
{
   public:
      NotifierFifo() {}
      NotifierFifo(ProcessNotifier::Handler* handler) : ProcessNotifier(handler) {}

      // Add a message to the fifo.
      virtual void add(Msg* msg)
      {
         Fifo<Msg>::add(msg);
         notify();
      }
};

}

#endif
