#ifndef RESIP_CongestionManager_hxx
#define RESIP_CongestionManager_hxx 

#include "rutil/Data.hxx"

namespace resip
{
class FifoStatsInterface;

/**
   @brief Abstract base class that provides a basic congestion management 
   interface.
   The primary job of subclasses is to implement 
   getRejectionBehavior() such that it will return 
   REJECTING_NEW_WORK or REJECTING_NON_ESSENTIAL when sufficient congestion is 
   detected in the fifo in question. logCurrentState() is used to provide a 
   logging statement for the current state of the queueing system.

   In order to write a CongestionManager that determines congestion state for 
   specific fifos based on the state of the whole system, you will need to 
   override getRejectionBehavior() to examine the fifo being passed to determine 
   what role it has in the system (this is done by calling 
   FifoStatsInterface::getDescription()), and assigning a role-number that 
   designates the role of this fifo (this means you only need to perform a 
   string comparison the first time getRejectionBehavior() is called for a 
   particular fifo). Here are the returns you will get from 
   FifoStatsInterface::getDescription() on a number of key fifos in the 
   resiprocate stack:

   - TransactionController::mStateMacFifo
   - TlsTransport::mTxFifo
   - TcpTransport::mTxFifo
   - UdpTransport::mTxFifo
   - DnsStub::mCommandFifo

   The following fifos don't see much use, and are therefore not that vital, but they do have descriptions.

   - SipStack::mTUFifo
   - TuSelector::mShutdownFifo

   @ingroup message_passing
*/
class CongestionManager
{

   public:

   /**
      Constructor
   */
   CongestionManager(){};
   virtual ~CongestionManager(){};

   typedef enum
   {
      NORMAL=0, /**< Not rejecting any work. */
      REJECTING_NEW_WORK=1, /**< Overloaded, should refuse new work. Should not 
                                 refuse continuation of old work. */
      REJECTING_NON_ESSENTIAL=2 /**< Rejecting all work that is non-essential to 
                                 the health of the system (ie, if dropping 
                                 something is liable to cause a leak, 
                                 instability, or state-bloat, don't drop it. 
                                 Otherwise, reject it.) */
   } RejectionBehavior;

   /**
      Return the current rejection behavior for this fifo.
      This is the primary functionality provided by this class. Implementors may
      merely examine the current state of the fifo in question, or they may make
      the decision based on the state of the entire system.
      @param fifo The fifo in question.
      @return The current desired RejectionBehavior for the fifo.
   */
   virtual RejectionBehavior getRejectionBehavior(const FifoStatsInterface *fifo) const=0;

   /**
      Registers a fifo with the congestion manager. May be a no-op, or may 
      assign a role number to the fifo.
   */
   virtual void registerFifo(resip::FifoStatsInterface* fifo)=0;

   /**
      Unregisters a fifo with the congestion manager.
   */
   virtual void unregisterFifo(resip::FifoStatsInterface* fifo)=0;

   /**
      Log the current state of all monitored fifos.
   */
   virtual void logCurrentState() const=0;

   /**
      Get the current state of all monitored fifos.to a stream
   */
   virtual EncodeStream& encodeCurrentState(EncodeStream& strm) const=0;

};


} // namespace resip

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
