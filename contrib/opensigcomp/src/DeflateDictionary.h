#ifndef __OSC__DEFLATE_DICTIONARY
#define __OSC__DEFLATE_DICTIONARY 1

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
  @file DeflateDictionary.h
  @brief Header file for osc::DeflateDictionary class.
*/


#include "Types.h"
#include "MultiBuffer.h"

namespace osc
{
  /**
    Used by the DeflateCompressor to index messages.

    The DeflateCompressor uses the DeflateDictionary to efficiently index
    information about historical messages so that distance/length pairs
    can be generated relatively rapidly. To support distance/length pairs
    in which the length exceeds the distance, the DeflateDictionary
    is additionally loaded with the message currently being
    compressed.

    Because the DeflateDictionary knows the contents of the
    message once it is initialized, the DeflateCompressor can simply
    repeatedly ask the DeflateDictionary for subsequent distance/length
    pairs without actually passing information about the specific data
    to be compressed
  */

  class DeflateDictionary
  {
    public:
      DeflateDictionary(size_t sizeHint = 8192);
      ~DeflateDictionary();

      bool isValid() { return (m_indexHead); }
 
      DeflateDictionary * operator &(){ return this; }
      DeflateDictionary const * operator &() const { return this; }

      bool addHistory(const byte_t *buffer, size_t size);
      bool addFuture(const byte_t *buffer, size_t size);

      bool findNextLengthAndDistance(unsigned int &length, 
                                     unsigned int &distance);

      osc::byte_t getCurrent() const { return m_data[m_curr]; }
      bool isFinished() const { return (m_curr == getSize()); }

      void increment (unsigned int size = 1);

      size_t getSize() const { return m_data.getSize(); }

      size_t getMaxDistance() const { return m_maxDistance; }
      void   setMaxDistance(size_t distance) { m_maxDistance = distance; }

      // Buffer access
      size_t getSize() { return m_data.getSize(); }
      osc::byte_t operator[](size_t index) const { return m_data[index]; }

    protected:

    private:
      bool addIndex();

      MultiBuffer m_data;
      size_t m_curr;
      size_t m_maxDistance;

      /**
        Offsets for the most recent occurance of each character (or pair
        of characters). For example, using OPTIMIZE_SIZE, if the most
        recent occurance of the character 'A' (ASCII 65) were at index
        postion 732, then m_indexHead[65] would be 732. The value 65535
        is reserved to mean 'does not exist'. When compiled with 
        "OPTIMIZE_SIZE," this array uses only the lowest 7 bits of
        the current character as the index. Otherwise, it uses
        a concatenation of the current character and its successor
        as the index.
       */
      osc::u16 *m_indexHead;


      /**
        This is a pointer to an array that has one element for each
        character in the entire MultiBuffer; each element in the array
        points to the previous occurance of the corresponding character
        (or set of characters) in the MultiBuffer. The value 65535 is
        reserved to mean 'does not exist.' For example, if the buffer
        contained the string "abbaa", then m_indexNext would be 5 elements
        long, and contain the values {65535, 65535, 1, 0, 3}.
      */
      osc::u16 *m_indexNext;

      /* if you define these, move them to public */
      DeflateDictionary(DeflateDictionary const &);
      DeflateDictionary& operator=(DeflateDictionary const &);
  };
}

#endif
