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
  @file State.h
  @brief Header file for osc::State class.
*/


#ifndef __OSC_STATE_H
#define __OSC_STATE_H 1
#include <assert.h>
#include "Types.h"
#include "Buffer.h"

#ifdef DEBUG
#include <ostream>
#endif

namespace osc
{
class StateHandler;

  /**
    Tracks the states stored and used by the UDVM and Compressors. 

    State objects are primarily a buffer and several attributes:

    @li Id - SHA-1 hash of the state's value, as described in RFC 3320.
    @li Length - number of bytes in the buffer
    @li Address - UDVM memory position into which the state is to be 
        loaded (unless an alternate location is specified).
    @li PC - Value of the program counter to be used if the state is to be
        executed.
    @li Minimum Access Length - Minimum number of bytes of the state ID 
        that must be specified to retrieve the state

    Additionally, the State objects have explicit reference
    counting. Whenever a State is created, it has a reference count of 0
    (this is the only time at which the reference count it allowed to
    be zero). Any time a State is added to a Compartment, its reference
    count is incremented.  When the State is removed from a Compartment,
    its reference count is decremented. Similarly, when a State is
    retrieved from the StateHandler, its reference count is incremented
    by the StateHandler. When the caller is finished with the State,
    it must call "release" on the State.

    The methods to increase and decrease the reference counts on State
    will lock the State for writing internally - this means that the
    caller of these methods must not be holding a lock on the object
    when such methods are called.
  */

class State
{
  public:
    State(osc::Buffer const &stateData, 
          osc::u16 address = 0,
          osc::u16 instruction = 0,
          osc::u16 minimumAccessLength = 6);

    State(osc::u16 address = 0,
          osc::u16 instruction = 0,
          osc::u16 minimumAccessLength = 6);

    virtual ~State();

    osc::byte_t *getMutableBuffer(size_t);
    void finalizeMutableBuffer();

    void setStaticBuffer(osc::byte_t *, size_t);
    
    bool isEquivalentTo(osc::State &state);
    size_t getStateSize() const;
    osc::count_t release();
    osc::count_t retain();
    void setCountObserver(osc::StateHandler * observer);

    void truncate(size_t newSize);

    bool matchesStateId(Buffer const &stateId);
    const osc::Buffer & getStateId() const;
    osc::Buffer &getStateData();
    const osc::byte_t *getStateDataRawBuffer() const;

    // Accessors
    osc::u16 getMinimumAccessLength() const { return m_minimumAccessLength; }
    osc::u16 getAddress() const { return m_address; }
    osc::u16 getInstruction() const { return m_pc; }
    const osc::Buffer &getId() const { return m_stateId; }

    osc::u16 getWord(osc::u16 address);

    // Private by contract: only called from StateHandler and related classes
    osc::count_t decrementUsageCount();
    osc::count_t incrementUsageCount();

#ifdef DEBUG
    // This is not neccessary, don't hook this for every state.
    // It really is only intended for debugging
    void setReferenceLeakObserver(osc::StateHandler &observer, 
                                  osc::count_t referenceMaximumThreshold);
    osc::count_t getRefCount() const { return m_retainCounter; }
    void dump(std::ostream &, unsigned int indent = 0) const;
    void diff(std::ostream &, osc::State &) const;
#endif

  protected:
    State(const osc::byte_t *stateStart,
          size_t stateSize,
          osc::u16 address = 0,
          osc::u16 instruction = 0,
          osc::u16 minimumAccessLength = 6);

  private:
    /**
      The buffer for state data
    */
    osc::Buffer m_buffer;

    /**
      The identifier for the state.
    */
    osc::byte_t m_stateIdArray[20];

    /**
      Convenience wrapper for m_stateIdArray.
    */
    osc::Buffer m_stateId;

    /**
      The default location in the UDVM into which this state will
      be loaded.
    */
    osc::u16 m_address;
    /**
      The value that the program counter will take if it is loaded.
    */
    osc::u16 m_pc;
    /**
      The minimum number of bytes required to address this state.
    */
    osc::u16 m_minimumAccessLength;
    
    osc::count_t m_retainCounter;

    osc::StateHandler * m_countObserver;
  
    void setStateId();
    
};

#ifdef DEBUG
std::ostream& operator<< (std::ostream &, const osc::State &);
#endif
}
#endif

