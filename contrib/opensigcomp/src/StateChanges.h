#ifndef __OSC__STATE_CHANGES
#define __OSC__STATE_CHANGES 1

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
  @file StateChanges.h
  @brief Header file for osc::StateChanges class.
*/

#include "Types.h"
#include "Buffer.h"

#ifdef DEBUG
#include <ostream>
#endif

namespace osc
{
  class State;

  /**
    Represents the state changes requested by a successfully decompressed
    SigComp message.

    If the application deems a decompressed message valid, it calls
    osc::Stack::provideCompartmentId with these changes to accept them
    (and assign them to a compartment).

    The state changes can contain a sequence of up to eight requests,
    four of which may be state additions and four of which may be state 
    deletions.

    The StateChanges class contains an array of 8 Operations. "Operation"
    is a "struct" defined within the scope of StateChanges. Each
    Operation is either an "Add" or a "Delete" operation. The "Delete"
    operations will contain a state identifier; the "Add" operations
    will contain a pointer to a new State object that is to be added to
    the state handler.
  */

  class StateChanges
  {
    public:
      typedef enum {PROTO_STATE, ADD_STATE, REMOVE_STATE} operation_t;

      StateChanges();
      ~StateChanges();
      osc::StateChanges * operator &(){ return this; }
      osc::StateChanges const * operator &() const { return this; }

      bool addProtoState(osc::u16 length,
                         osc::u16 address,
                         osc::u16 instruction,
                         osc::u16 accessLength,
                         osc::u16 priority);
      bool addState(osc::State *, osc::u16 priority = 65535);
      bool deleteState(osc::state_id_t const &);

      void setState(osc::u8 index, osc::State *);

      bool hasNextOperation() const;
      bool isNextOperationAdd() const;

      osc::u8 getNumOperations() const;
      operation_t getOperationType(osc::u8 index) const;

      // For PROTO_STATE and ADD_STATE
      osc::u16 getPriority(osc::u8 index) const;

      // For PROTO_STATE
      osc::u16 getLength(osc::u8 index) const;
      osc::u16 getAddress(osc::u8 index) const;
      osc::u16 getInstruction(osc::u8 index) const;
      osc::u16 getAccessLength(osc::u8 index) const;

      // For ADD_STATE
      osc::State* getState(osc::u8 index);

      // For REMOVE_STATE
      osc::state_id_t &getStateId(osc::u8 index);

      // Returned Sigcomp Parameters
      void setCpbDmsSms(osc::u8 cpbDmsSms) { m_cpbDmsSms = cpbDmsSms;}
      void setRemoteSigcompVersion(osc::u16 version ) { m_version = version; }
      void addRemoteState(osc::state_id_t id) 
        {if (m_numRemoteStates < 8) {m_remoteState[m_numRemoteStates++] = id;}}

      osc::u8  getCpbDmsSms() const { return m_cpbDmsSms;}
      osc::u16 getRemoteSigcompVersion() const { return m_version; }
      osc::u16 getNumRemoteStates() const { return m_numRemoteStates; }
      osc::state_id_t &getRemoteState(osc::u16 i) { return m_remoteState[i]; }
      const osc::state_id_t &getRemoteState(osc::u16 i) const 
        { return m_remoteState[i]; }

      // Requested Feedback to Send
      void setIBitValid(bool i = true) { m_iBitIsValid = i; }
      void setSBit(bool s = true) { m_sBit = s;}
      void setIBit(bool i = true) { m_iBit = i;}
      void setRequestedFeedback(const osc::byte_t *start, size_t length)
        {m_requestedFeedback.copy(start, length);}

      bool iBitIsValid() const { return m_iBitIsValid; }
      bool getSBit() const { return m_sBit; }
      bool getIBit() const { return m_iBit; }
      osc::Buffer &getRequestedFeedback()
        { return m_requestedFeedback; }
      const osc::Buffer &getRequestedFeedback() const
        { return m_requestedFeedback; }

      // Returned feedback
      void setReturnedFeedback(const osc::byte_t *start, size_t length)
        {m_returnedFeedback.copy(start, length);}
      osc::Buffer &getReturnedFeedback()
        { return m_returnedFeedback; }
      const osc::Buffer &getReturnedFeedback() const
        { return m_returnedFeedback; }

#ifdef DEBUG
      void dump(std::ostream &, unsigned int indent = 0) const;
#endif

    protected:
      /**
        Represents a single action to be taken upon message acceptance.
      */
      typedef struct
      {
        operation_t operation;
        osc::state_id_t deleteStateId;
        union
        {
          struct
          {
            osc::u16        priority;
            osc::State *    state;
          } newState;

          struct
          {
            osc::u16 priority;
            osc::u16 length;
            osc::u16 address;
            osc::u16 instruction;
            osc::u16 accessLength;
          } protoState;
        } info;
      }
      state_change_operation_t;

      state_change_operation_t m_operations[8];

      osc::u8 m_deleteCount;
      osc::u8 m_addCount;
      osc::u8 m_operationIndex;

      // Returned Sigcomp Parameters
      osc::u8  m_cpbDmsSms;
      osc::u16 m_version;
      osc::u16 m_numRemoteStates;

      /**
        @note We hardcode this to 8. If the remote endpoint
              goes bonkers and wants to advertise more than 8
              states, fine. But we're not going to look past
              the first 8.
      */
      osc::state_id_t m_remoteState[8];

      // Requested feedback parameters
      bool m_iBitIsValid;
      bool m_sBit;
      bool m_iBit;
      osc::Buffer m_requestedFeedback;
      osc::Buffer m_returnedFeedback;

    private:
      /* if you define these, move them to public */
      StateChanges(osc::StateChanges const &);
      osc::StateChanges& operator=(osc::StateChanges const &);
  };


#ifdef DEBUG
  std::ostream& operator<< (std::ostream &, const osc::StateChanges &);
#endif
}

inline osc::u8 
osc::StateChanges::getNumOperations() const 
{ 
  return m_operationIndex; 
}

inline void
osc::StateChanges::setState(osc::u8 index, osc::State *state)
{
  m_operations[index].operation = ADD_STATE;
  m_operations[index].info.newState.state = state;
}

inline osc::StateChanges::operation_t
osc::StateChanges::getOperationType(osc::u8 index) const
{
  return m_operations[index].operation;
}

inline osc::u16
osc::StateChanges::getPriority(osc::u8 index) const
{
  return m_operations[index].info.newState.priority;
}

inline osc::u16
osc::StateChanges::getLength(osc::u8 index) const
{
  return m_operations[index].info.protoState.length;
}

inline osc::u16
osc::StateChanges::getAddress(osc::u8 index) const
{
  return m_operations[index].info.protoState.address;
}

inline osc::u16
osc::StateChanges::getInstruction(osc::u8 index) const
{
  return m_operations[index].info.protoState.instruction;
}

inline osc::u16
osc::StateChanges::getAccessLength(osc::u8 index) const
{
  return m_operations[index].info.protoState.accessLength;
}

inline osc::State *
osc::StateChanges::getState(osc::u8 index)
{
  return m_operations[index].info.newState.state;
}

inline osc::state_id_t &
osc::StateChanges::getStateId(osc::u8 index)
{
  return m_operations[index].deleteStateId;
}

#endif
