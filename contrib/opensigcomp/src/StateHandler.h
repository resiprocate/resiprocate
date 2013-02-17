
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
  @file StateHandler.h
  @brief Header file for osc::StateHandler class.
*/


#ifndef __OSC_STATE_Handler_H
#define __OSC_STATE_Handler_H 1
#include "StateMap.h"
#include "SigcompMessage.h"
#include "ReadWriteLockable.h"
#include "CompartmentMap.h"

namespace osc
{

  class StateChanges;
  class State;
  class Compartment;
  class NackMap;
  
  /**
    Stores state for the UDVM and tracks NACK messages and compartments.

    The StateHandler class is responsible for the destruction of
    old states and compartments, which it handles through reference
    counting. When ever a state or compartment is retrieved from the
    StateHandler it must be released when the retriever is finished
    using it.

    The StateHandler also provides a method of removing stale
    compartments; by calling removeStaleCompartments all stale
    compartments will be discarded from the state handler and all the
    state that was unique to those compartments will be discarded
    as well. A stale compartment is defined with in the context of
    StateHandler as a Compartment that has neither been retained nor
    released since the last time removeStaleCompartments was called.

    NACK to compartment associations are autmatically removed by age
    once a threshold of associations is met. This threshold is computed
    by multiplying a the per-compartment threshold by the number of
    compartments. Because of this, there is no guaranteed minimum number
    of associations that will be stored per-compartment.

    @see RFC 3320 Section 6
  */
  class StateHandler
  {
    public:
      StateHandler (size_t maxStateSpaceInBytes = 8192, 
                    size_t maximum_cyclesPerBit = 64,
                    size_t maxDecompressionSpaceInBytes = 8192,
                    osc::u16 sigcompVersion = 0x02, 
                    osc::count_t nackThreshold=4,
                    unsigned int assumeRemoteSms = 2048,
                    unsigned int assumeRemoteCpb = 16,
                    unsigned int assumeRemoteDms = 8192,
                    osc::u16 assumeRemoteVersion = 1);

      ~StateHandler ();
      
      osc::count_t numberOfStates ();
      osc::count_t numberOfCompartments ();
      osc::State *addState (osc::State *);
      size_t addState (osc::State *, 
                       osc::compartment_id_t const &,
                       osc::u16 priority);
      osc::State * getState (osc::state_id_t const &stateId,
                             osc::nack_code_t &status);
    
      osc::Compartment * getCompartment (osc::compartment_id_t const &);
      osc::Compartment * findNackedCompartment (const osc::sha1_t &);

      void addNack(osc::sha1_t &, osc::Compartment * compartment);

      osc::count_t numberOfNacks();
      void removeStaleCompartments ();

      void processChanges (osc::StateChanges &, osc::Compartment &);
      
      osc::count_t releaseState (osc::State * state);
      osc::count_t retainState (osc::State * state);
      osc::count_t releaseCompartment (osc::Compartment * compartment);
      osc::count_t releaseCompartmentNack (osc::Compartment * compartment);

      void compartmentCounterDepleted (osc::Compartment * compartment);
    
      void useSipDictionary();
      bool hasSipDictionary() { return m_hasSipDictionary; }
      
      
      osc::u32 getDecompressionMemorySize ()
      {
        return m_parameters.decompression_memory_size;
      }
    
      osc::u32 getStateMemorySize ()
      {
        return m_parameters.state_memory_size;
      }
    
      osc::u8 getCyclesPerBit ()
      {
        return m_parameters.cycles_per_bit;
      }
    
      osc::u16 getSigcompVersion ()
      {
        return m_parameters.SigComp_version;
      }
    
      osc::byte_t getCpbDmsSms();
    
#ifdef DEBUG
      void processChanges (osc::StateChanges &, osc::compartment_id_t const &);

      void addNack(osc::SigcompMessage &msg, osc::Compartment * compartment)
      {
        osc::sha1_t sha1;
        msg.getSha1Hash(sha1.digest,20);
        addNack(sha1, compartment);
      }

      void dump(std::ostream &out, unsigned int indent);
#endif 

    protected:
      osc::sigcomp_parameters_t m_parameters;
      osc::StateMap m_states;
      osc::CompartmentMap m_compartments;
      bool m_hasSipDictionary;
    
    private:
      osc::NackMap *m_nacks;
      osc::count_t m_nackThreshold;
      osc::ReadWriteLockable m_nackLock;
  };
}
#endif

