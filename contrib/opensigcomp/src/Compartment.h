#ifndef __OSC__COMPARTMENT
#define __OSC__COMPARTMENT 1

/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2005 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

/**
  @file Compartment.h
  @brief Header file for osc::Compartment class.
*/



#include "ReadWriteLockable.h"
#include "Types.h"
#include "SigcompMessage.h"
#include "StateMap.h"
#include "CompressorData.h"
#include "StateList.h"

#ifdef LEAK_DEBUG
#include "LeakDebug.h"
#endif

#ifdef DEBUG
#include <ostream>
#endif

extern int numComp;

namespace osc
{
  class StateHandler;
  class State;

  /**
    Tracks information for a single remote endpoint.

    The Compartment is responsible for tracking all the information
    associated with a remote endpoint. This includes SigComp state
    objects (see osc::State), information about the amount of memory
    and advertised states available in the remote SigComp implementation,
    and any information that the compressor must store. See
    osc::CompressorData for information about compressor data storage.

    The compartment itself is responsible for ensuring that the space
    consumed by the requested states does not exceed the maximum amount
    allowed, and will release state objects as necessary. The order of
    releasing such objects is described in RFC 3320. At a high level,
    lower priority states are always removed before higher priority
    states, and states are removed on a least recently created basis
    (not least recently used).

    The compartment lifecycle is managed by two reference counters.
    The first, m_nackRetainCount, tracks how many times the compartment
    is referenced in the NackMap structure. The second, m_retainCount,
    counts how many times it is referenced elsewhere. Only when both
    counts go to zero is the comparment deallocated.

    Whenever a Compartment is created, it has an m_retainCount count of 1;
    this represents that it is being referenced by the StateHandler. Any
    time a Compartment is retrieved from the StateHandler, its reference
    count is incremented by the StateHandler. When the caller is finished
    with the Compartment, it must call "release" on the Compartment.

    The methods to increase and decrease the reference counts on
    Compartment lock the Comparment for writing internally -- this
    means that the caller of these methods must not be holding a lock
    on the object when such methods are called.
   */
  
class Compartment : public osc::ReadWriteLockable
  {
    public:
      Compartment(osc::compartment_id_t const &compartmentId,
                  size_t maxStateSpace,
                  osc::u8  assumeCpbDmsSms = 0x19,
                  osc::u16 assumeVersion = 1
                  );

      ~Compartment();
  
      /**
        @returns a pointer to this.
       */
      Compartment * operator &(){ return this; }
  
      /**
        @returns a pointer to this.
        */
      osc::Compartment const * operator &() const { return this; }
  
      void                      removeState(osc::state_id_t const &stateId);
      void                      addState(osc::State *state, osc::u16 priority);

      osc::count_t              release();
      osc::count_t              retain();
      osc::count_t              nackRelease();
      osc::count_t              nackRetain();
      size_t                    remainingStateMemory();
      size_t                    totalStateMemoryAvailable();
      osc::compartment_id_t     getCompartmentId() const;
      bool                      isStale() const {return m_stale;}
      void                      setStale(bool stale = true) {m_stale = stale;}
      bool                      isZombie() const {return m_zombie;}
      osc::count_t              getRetainCount() const;
      void                      setCountObserver(osc::StateHandler * observer);
      bool                      addCompressorData(osc::CompressorData *data);
      osc::CompressorData * getCompressorData(osc::compressor_id_t
                                              compressorId=0);
      osc::u32 getLookup2Hash();

      osc::count_t decrementRetainCount();
      osc::count_t decrementNackRetainCount();

      // Handling for states we expect the remote compartment to know about
      void        addRemoteState(osc::State *state, osc::u16 priority,
                                 bool acked);
      void        removeRemoteState(osc::state_id_t const &stateId);
      void        ackRemoteState(osc::state_id_t const &stateId);
      bool        isRemoteStateAcked(osc::state_id_t const &stateId);
      void        setRemoteStateMemorySize(size_t quota);
      osc::State *getMostRecentAckedState(osc::u16 priority);
      void        resetRemoteStates();
    


      // Returned Sigcomp Parameters
      void setCpbDmsSms(osc::u8 cpbDmsSms);
      void setRemoteSigcompVersion(osc::u16 version ) { m_version = version; }
      void addRemoteAdvertizedState(osc::state_id_t &id);
      void resetRemoteAdvertizedStates() { m_numRemoteAdvertizedStates = 0; }

      osc::u16 getRemoteCyclesPerBit() const;
      osc::u32 getRemoteDecompressionMemorySize() const;
      osc::u32 getRemoteStateMemorySize() const;
      osc::byte_t getRemoteCpbDmsSms() const { return m_cpbDmsSms; }
      osc::u16 getRemoteSigcompVersion() const { return m_version; }
      osc::u16 getNumRemoteAdvertizedStates() const 
        { return m_numRemoteAdvertizedStates; }
      osc::state_id_t getRemoteState(osc::u16 i) const
        { return m_remoteAdvertizedState[i]; }

      // Requested Feedback (to be sent)
      void setIBit(bool i = true) { m_iBit = i;}
      void setRequestedFeedback(osc::Buffer &feedback)
        {m_requestedFeedback.subsume(feedback);}
      void freeRequestedFeedback()
        {m_requestedFeedback.free();}

      bool getIBit() const { return m_iBit; }
      const osc::Buffer &getRequestedFeedback() const
        { return m_requestedFeedback; }

      // Returned feedback (received from remote endpoint)
      void setReturnedFeedback(osc::Buffer &feedback)
        {m_returnedFeedback.subsume(feedback);}
      const osc::Buffer &getReturnedFeedback() const
        { return m_returnedFeedback; }

      // Is the transport reliable?
      bool isTransportReliable() const { return m_reliableTransport; }
      void setReliableTransport(bool f = true) { m_reliableTransport = f; }

      static osc::u8 makeCpbDmsSms(unsigned int cpb, 
                                   unsigned int dms,
                                   unsigned int sms);

      static unsigned int getCpbFromByte(osc::u8 byte)
        {return (16 << (byte >> 6));}

      static unsigned int getDmsFromByte(osc::u8 byte)
        {unsigned int dms = ((byte >> 3) & 0x07); return dms?(1024<<dms):2048;}

      static unsigned int getSmsFromByte(osc::u8 byte)
        {unsigned int sms = (byte & 0x07); return sms?(1024<<sms):0;}

#ifdef DEBUG
      void dump(std::ostream &, unsigned indent = 0) const; 
#endif
      
    private:
      osc::StateList m_localStates;
      osc::StateList m_remoteStates;

      osc::count_t m_retainCount;
      osc::count_t m_nackRetainCount;
      
      osc::StateHandler * m_countObserver;

      // Compressor Data Related Structures
      osc::CompressorData ** m_data;
      size_t m_dataTop;
      size_t m_dataCount;

      osc::compartment_id_t m_compartmentId;
      bool m_stale;
      bool m_zombie;
      osc::u32 m_lookup2Hash;
      
      // Returned SigComp parameters
      osc::u8  m_cpbDmsSms;
      osc::u16 m_version;
      osc::u16 m_numRemoteAdvertizedStates;
      osc::state_id_t m_remoteAdvertizedState[8];

      // Requested feedback parameters
      bool m_iBit;
      osc::Buffer m_requestedFeedback;
      osc::Buffer m_returnedFeedback;

      // Flag: Does this compartment correspond to a reliable
      // mechanism?
      bool m_reliableTransport;

      /* if you define these, move them to public */
      Compartment(osc::Compartment const &);
      osc::Compartment& operator=(osc::Compartment const &);


#ifdef DEBUG
    public:
      /**
        @returns the number of states that the compartment holds
        @note For debugging use only -- not particularly efficient.
      */
      osc::count_t numberOfStates() {return m_localStates.numberOfStates();}
#endif
#ifdef LEAK_DEBUG
    DEBUG_LEAK_HEADER_HOOK
#endif
  };
#ifdef DEBUG
  std::ostream &operator<<(std::ostream &, const osc::Compartment &);
#endif
} 

inline void
osc::Compartment::addRemoteAdvertizedState(osc::state_id_t &id)
{
  if (m_numRemoteAdvertizedStates < 8) 
  {
    m_remoteAdvertizedState[m_numRemoteAdvertizedStates++].subsume(id);
  }
}

inline osc::u16
osc::Compartment::getRemoteCyclesPerBit() const
{
  return (16 << (m_cpbDmsSms >> 6));
}

inline osc::u32
osc::Compartment::getRemoteDecompressionMemorySize() const
{
  osc::u8 bits = (m_cpbDmsSms >> 3) & 0x07;
  if (!bits)
  {
    return 0;
  }
  return (1024 << bits);
}

inline osc::u32
osc::Compartment::getRemoteStateMemorySize() const
{
  osc::u8 bits = m_cpbDmsSms & 0x07;
  if (!bits)
  {
    return 0;
  }
  return (1024 << bits);
}

#endif
