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
  @file StateChanges.cpp
  @brief Implementation of osc::StateChanges class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "StateChanges.h"
#include "State.h"
#include "Types.h"

#ifdef DEBUG
#include <iomanip>
#endif

/** 
  Defalut constructor
 */
osc::StateChanges::StateChanges() : 
    m_deleteCount(0),
    m_addCount(0),
    m_operationIndex(0),
    m_cpbDmsSms(0),
    m_version(0),
    m_numRemoteStates(0),
    m_iBitIsValid(false),
    m_sBit(false),
    m_iBit(false)
{
  DEBUG_STACK_FRAME;
}

/**
  removes a state with the given Id from the StateMap

*/
bool osc::StateChanges::deleteState(osc::state_id_t const &delState)
{
  DEBUG_STACK_FRAME;
  if(m_deleteCount<=3)
  {
    m_operations[m_operationIndex].operation = REMOVE_STATE;
    m_operations[m_operationIndex].deleteStateId = delState;
    m_operationIndex++;
    m_deleteCount++;
    return true;
  }
  return false;
}

/**
  @param state
*/
bool osc::StateChanges::addState(osc::State *state,
                                 osc::u16 priority)
{
  DEBUG_STACK_FRAME;
  if(m_addCount<=3)
  {
    m_operations[m_operationIndex].operation = ADD_STATE;
    m_operations[m_operationIndex].info.newState.state = state;
    m_operations[m_operationIndex].info.newState.priority = priority;
    m_operationIndex++;
    m_addCount++;
    return true;
  }
  return false;
}

bool osc::StateChanges::addProtoState(osc::u16 length,
                                      osc::u16 address,
                                      osc::u16 instruction,
                                      osc::u16 accessLength,
                                      osc::u16 priority)
{
  DEBUG_STACK_FRAME;
  if(m_addCount<=3)
  {
    m_operations[m_operationIndex].operation = PROTO_STATE;
    m_operations[m_operationIndex].info.protoState.priority = priority;
    m_operations[m_operationIndex].info.protoState.length = length;
    m_operations[m_operationIndex].info.protoState.address = address;
    m_operations[m_operationIndex].info.protoState.instruction = instruction;
    m_operations[m_operationIndex].info.protoState.accessLength = accessLength;
    m_operationIndex++;
    m_addCount++;
    return true;
  }
  return false;
}

osc::StateChanges::~StateChanges()
{
  DEBUG_STACK_FRAME;
  for (osc::u8 i=0; i < m_operationIndex; i++)
  {
    if (m_operations[i].operation == ADD_STATE)
    {
      delete m_operations[i].info.newState.state;
    }
  }
}

/**
  Assignment operator for osc::StateChanges.
 */
osc::StateChanges & osc::StateChanges::operator=(StateChanges const &r)
{
  DEBUG_STACK_FRAME;
  assert(0);
  if (&r == this)
  {
    return *this;
  }
  return *this;
}

/// Don't copy these
osc::StateChanges::StateChanges(osc::StateChanges const &r)
{
  DEBUG_STACK_FRAME;
  assert(0);
}

#ifdef DEBUG
void
osc::StateChanges::dump(std::ostream &os, unsigned int indent) const
{
  DEBUG_STACK_FRAME;
  osc::u8 i;
  os << std::setw(indent) << ""
     << "[StateChanges " << this << "]" << std::endl
     << std::setw(indent) << ""
     << "  # Operations = " << static_cast<unsigned int>(m_operationIndex) 
                            << std::endl
     << std::setw(indent) << ""
     << "  # Adds       = " << static_cast<unsigned int>(m_addCount) 
                            << std::endl
     << std::setw(indent) << ""
     << "  # Deletes    = " << static_cast<unsigned int>(m_deleteCount) 
                            << std::endl;
  for (i=0; i<m_operationIndex; i++)
  {
     os << std::setw(indent) << "";
     os << "    Operation " << static_cast<unsigned int>(i) << " = ";
     switch (m_operations[i].operation)
     {
       case PROTO_STATE: 
         os << "PROTO_STATE" << std::endl; 
         os << std::setw(indent) << "";
         os << "    Priority    = " 
            << m_operations[i].info.protoState.priority << std::endl;
         os << std::setw(indent) << "";
         os << "    Length      = " 
            << m_operations[i].info.protoState.length << std::endl;
         os << std::setw(indent) << "";
         os << "    Address     = " 
            << m_operations[i].info.protoState.address << std::endl;
         os << std::setw(indent) << "";
         os << "    Instruction = " 
            << m_operations[i].info.protoState.instruction << std::endl;
         os << std::setw(indent) << "";
         os << "    Acc. Length = " 
            << m_operations[i].info.protoState.accessLength << std::endl;
       break;
       case ADD_STATE: 
         os << "ADD_STATE" << std::endl;
         os << std::setw(indent) << "";
         os << "    Priority    = " 
            << m_operations[i].info.newState.priority << std::endl;
         m_operations[i].info.newState.state->dump(os, indent + 4);
       break;

       case REMOVE_STATE: 
         os << "REMOVE_STATE" << std::endl; 
         os << std::setw(indent) << "";
         os << "    State ID    = " 
            << m_operations[i].deleteStateId << std::endl;
       break;
       default:
         os << "!!! BAD VALUE !!!" << std::endl; 
     }
     os << std::endl;
  }

  unsigned int cpb = 16 << (m_cpbDmsSms >> 6);
  unsigned int dms = ((m_cpbDmsSms >> 3) & 0x07);
  dms = dms?1024 << dms:0;
  unsigned int sms = (m_cpbDmsSms & 0x07);
  sms = sms?1024 << sms:0;

  os << std::setw(indent) << ""
     << "  CPB/DMS/SMS  = " << static_cast<unsigned int>(m_cpbDmsSms) 
     << " (" << cpb << "/" << dms << "/" << sms << ")"
     << std::endl;

  os << std::setw(indent) << ""
     << "  Version      = " << static_cast<unsigned int>(m_version)
     << std::endl;

  os << std::setw(indent) << ""
     << "  # Remote St. = " << static_cast<unsigned int>(m_numRemoteStates)
     << std::endl;

  for (i = 0; i < m_numRemoteStates; i++)
  {
    os << std::setw(indent) << ""
       << "    Remote State [" << static_cast<unsigned int>(i) 
       << "] = " << m_remoteState[i] << std::endl;
  }

  os << std::endl << std::setw(indent) << ""
     << "  Req FB (send)= " << m_requestedFeedback << std::endl;
  os << std::setw(indent) << ""
     << "  I Bit Valid  = " << m_iBitIsValid << std::endl;
  os << std::setw(indent) << ""
     << "  I Bit        = " << m_iBit << std::endl;
  os << std::setw(indent) << ""
     << "  S Bit        = " << m_sBit << std::endl;

  os << std::endl << std::setw(indent) << ""
     << "  Returned FB  = " << m_returnedFeedback << std::endl;
}

std::ostream &
osc::operator<< (std::ostream &os, const osc::StateChanges &sc)
{
  DEBUG_STACK_FRAME;
  sc.dump(os);
  return os;
}
#endif
