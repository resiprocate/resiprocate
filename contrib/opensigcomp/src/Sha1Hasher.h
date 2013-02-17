#ifndef __OSC__SHA1_HASHER
#define __OSC__SHA1_HASHER 1

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
  @file Sha1Hasher.h
  @brief Header file for osc::Sha1Hasher class.
*/


#include "Types.h"

#ifdef USE_OPENSSL_SHA1
#include "openssl/sha.h"
#endif

namespace osc
{
  /**
    Performs SHA-1 hashing as defined in FIPS PUB 180-1.

    Users of the Sha1Hasher simply instantiate an instance of the
    Sha1Hasher, call "addData" (perhaps repeatedly, if more than
    one buffer is to be checksummed), and then call "getHash" to
    retrieve the SHA-1 hash of the data.

    A substantial portion of the computational complexity of
    running SigComp comes from generating SHA-1 hashes. If the
    Open SSL library (libcrypto) is available, this class can take
    advantage of its hand-tweaked assembly SHA-1 implementation.
    Ensure that USE_OPENSSL_SHA1 is defined if you wish to take
    advantage of this speed enhancement.

    @note This implementation only handles whole bytes, and only
          handles blocks of input data up to 2^32 bits in length.
  */

  class Sha1Hasher
  {
    public:
      Sha1Hasher();
      ~Sha1Hasher();
 
      Sha1Hasher * operator &(){ return this; }
      Sha1Hasher const * operator &() const { return this; }

      void addData(const byte_t *buffer, size_t size);
      void getHash(byte_t[20], size_t hashLength = 20);

      void addData(osc::u16);

      void reset();

    protected:

    private:
      void hashBlock();

      union
      {
        struct
        {
          u32 h[5];
          u32 w[16];
          u32 length;
        } context;
#ifdef USE_OPENSSL_SHA1
        SHA_CTX ctx;
#endif
        u32 padding[28];
      } m_c;

      enum
      {
        K0_19  = 0x5A827999,
        K20_39 = 0x6ED9EBA1,
        K40_59 = 0x8F1BBCDC,
        K60_79 = 0xCA62C1D6
      };

      /* if you define these, move them to public */
      Sha1Hasher(Sha1Hasher const &);
      Sha1Hasher& operator=(Sha1Hasher const &);
  };
};

#endif
