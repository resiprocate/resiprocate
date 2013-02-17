#ifndef __OSC__SIP_DICTIONARY
#define __OSC__SIP_DICTIONARY 1

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
  @file SipDictionary.h
  @brief Header file for osc::SipDictionary class.
*/


#include "State.h"

namespace osc
{
  /**
    Contains the SIP/SDP dictionary defined in RFC 3485.
  */

  class SipDictionary : public State
  {
    public:
      SipDictionary();
      virtual ~SipDictionary();
 
      SipDictionary * operator &(){ return this; }
      SipDictionary const * operator &() const { return this; }

      bool findIndex(const osc::byte_t *input, size_t inputSize,
                     size_t &index, size_t &length) const;

      bool findOffset(const osc::byte_t *input, size_t inputSize,
                      size_t &offset, size_t &length) const;

      bool buildIndexTable();

      bool buildOffsetTable();

#ifdef DEBUG
      void dump(std::ostream &, unsigned int indent = 0) const;
#endif

    protected:

     enum {TABLE_OFFSET = 0x0D8C};

     osc::u16 *m_indexTableHead;
     osc::u16 *m_indexTable;

     osc::u16 *m_offsetTableHead;
     osc::u16 *m_offsetTableNext;

    private:
      /* if you define these, move them to public */
      SipDictionary(SipDictionary const &);
      SipDictionary& operator=(SipDictionary const &);

      static osc::byte_t s_stateValue[];
  };

#ifdef DEBUG
  std::ostream& operator<< (std::ostream &, const osc::SipDictionary &);
#endif

}

#endif
