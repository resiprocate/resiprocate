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
  @file Sha1Hasher.cpp
  @brief Implementation of osc::Sha1Hasher class.
*/


#include "ProfileStack.h"
#include <assert.h>
#include "Libc.h"
#include "Sha1Hasher.h"

// The following macros and/or includes provide ntohl and htonl.
#if defined(_MSC_VER) || defined (__MINGW32__)
  #if defined(_M_IX86) || defined(__i386__) || defined(LITTLE_ENDIAN)
    #define htonl(x) (((((unsigned long)(x) & 0xFF)) << 24) | \
                      ((((unsigned long)(x) & 0xFF00)) << 8) | \
                      ((((unsigned long)(x) & 0xFF0000)) >> 8) | \
                      ((((unsigned long)(x) & 0xFF000000)) >> 24))
    #define ntohl(x) htonl(x)
  #else
    #define htonl(x) (x)
    #define ntohl(x) (x)
  #endif
#else
#include <netinet/in.h>
#endif

// FIPS 180 defines the following functions: 
//
// #define F0_19(b,c,d)  (((b)&(c))|(~(b)&(d)))
// #define F40_59(b,c,d) (((b)&(c))|((b)&(d))|((c)&(d)))
//
// However, these are bit equivalent to (but slower than)
// What we use below. These optimizations are attributed to
// Rich Schroeppel and Wei Dai, respectively.

#define F0_19(b,c,d)  ((((c)^(d))&(b))^(d))
#define F20_39(b,c,d) ((b)^(c)^(d))
#define F40_59(b,c,d) (((b)&(c))|(((b)|(c))&(d)))
#define F60_79(b,c,d) ((b)^(c)^(d))

#define ROTATE(distance,word) (((word)<<(distance))|((word)>>(32-(distance))))

#define PASS_0_15(a,b,c,d,e,w) \
  e += ROTATE(5,a) + F0_19(b,c,d) + w + K0_19; \
  b = ROTATE(30,b);

#define PASS_16_19(a,b,c,d,e,w0,w13,w8,w2) \
  w0 = ROTATE(1, w13 ^ w8 ^ w2 ^ w0); \
  e += ROTATE(5,a) + F0_19(b,c,d) + w0 + K0_19; \
  b = ROTATE(30,b);

#define PASS_20_39(a,b,c,d,e,w0,w13,w8,w2) \
  w0 = ROTATE(1, w13 ^ w8 ^ w2 ^ w0); \
  e += ROTATE(5,a) + F20_39(b,c,d) + w0 + K20_39; \
  b = ROTATE(30,b);

#define PASS_40_59(a,b,c,d,e,w0,w13,w8,w2) \
  w0 = ROTATE(1, w13 ^ w8 ^ w2 ^ w0); \
  e += ROTATE(5,a) + F40_59(b,c,d) + w0 + K40_59; \
  b = ROTATE(30,b);

#define PASS_60_79(a,b,c,d,e,w0,w13,w8,w2) \
  w0 = ROTATE(1, w13 ^ w8 ^ w2 ^ w0); \
  e += ROTATE(5,a) + F60_79(b,c,d) + w0 + K60_79; \
  b = ROTATE(30,b);


/**
  Constructor for osc::Sha1Hasher.
 */
osc::Sha1Hasher::Sha1Hasher()
{
  DEBUG_STACK_FRAME;
  reset();
}

/**
  Copy constructor for osc::Sha1Hasher.
 */
osc::Sha1Hasher::Sha1Hasher(Sha1Hasher const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::Sha1Hasher.
 */
osc::Sha1Hasher::~Sha1Hasher()
{
  DEBUG_STACK_FRAME;
}

/**
  Assignment operator for osc::Sha1Hasher.
 */
osc::Sha1Hasher &
osc::Sha1Hasher::operator=(Sha1Hasher const &r)
{
  DEBUG_STACK_FRAME;
  if (&r == this)
  {
    return *this;
  }
  /* Assign attributes */
  assert(0);
  return *this;
}

#ifdef USE_OPENSSL_SHA1
#include "openssl/sha.h"
void
osc::Sha1Hasher::reset()
{
  DEBUG_STACK_FRAME;
  assert (sizeof(m_c.ctx) <= sizeof (m_c.padding));
  SHA1_Init(&m_c.ctx);
}

void
osc::Sha1Hasher::addData(const osc::byte_t *buffer, size_t size)
{
  DEBUG_STACK_FRAME;
  SHA1_Update(&m_c.ctx, buffer, size);
}

void
osc::Sha1Hasher::addData(osc::u16 foo)
{
  DEBUG_STACK_FRAME;
  unsigned char buffer[2];
  buffer[0] = foo >> 8;
  buffer[1] = foo & 0xff;
  SHA1_Update(&m_c.ctx, buffer, 2);
}

void
osc::Sha1Hasher::getHash(osc::byte_t hash[20], size_t hashLength)
{
  DEBUG_STACK_FRAME;
  if (hashLength >= 20)
  {
    SHA1_Final(hash, &m_c.ctx);
  }
  else
  {
    unsigned char h[20];
    SHA1_Final(h, &m_c.ctx);
    OSC_MEMMOVE(hash, h, hashLength);
  }
}
#else

void
osc::Sha1Hasher::reset()
{
  DEBUG_STACK_FRAME;
  assert (sizeof(m_c.context) <= sizeof (m_c.padding));
  m_c.context.h[0] = 0x67452301;
  m_c.context.h[1] = 0xEFCDAB89;
  m_c.context.h[2] = 0x98BADCFE;
  m_c.context.h[3] = 0x10325476;
  m_c.context.h[4] = 0xC3D2E1F0;
  m_c.context.length = 0;
}

void
osc::Sha1Hasher::addData(const osc::byte_t *buffer, size_t size)
{
  DEBUG_STACK_FRAME;
  unsigned int i;
  int block_len = m_c.context.length % 64;
  char *block = reinterpret_cast<char *>(m_c.context.w);

  for(i = 0; i < size; i++)
  {
    block[block_len] = buffer[i];
    block_len++;
    if (block_len == 64)
    {
      hashBlock();
      block_len = 0;
    }
  }

  m_c.context.length += size;
}

void
osc::Sha1Hasher::addData(osc::u16 foo)
{
  DEBUG_STACK_FRAME;
  int block_len = m_c.context.length % 64;
  osc::byte_t *block = reinterpret_cast<osc::byte_t *>(m_c.context.w);


  // Hash first byte
  block[block_len] = static_cast<osc::byte_t>(foo >> 8);
  block_len++;
  if (block_len == 64)
  {
    hashBlock();
    block_len = 0;
  }

  // Hash second byte
  block[block_len] = static_cast<osc::byte_t>(foo & 0xFF);
  block_len++;
  if (block_len == 64)
  {
    hashBlock();
    block_len = 0;
  }

  m_c.context.length += 2;
}

void
osc::Sha1Hasher::getHash(osc::byte_t hash[20], size_t hashLength)
{
  DEBUG_STACK_FRAME;
  int message_len_in_bits = m_c.context.length * 8;
  byte_t *block = reinterpret_cast<byte_t *>(m_c.context.w);
  int padsize;

  byte_t trailer = 0x80;
  addData(&trailer, sizeof(trailer));

  int block_len = m_c.context.length % 64;

  /* We need two words for the length information;
     if there's not enough room, we must pad this block
     until it is full and place the information in the
     next block. */
  if (block_len > 56)
  {
    padsize = 64 - block_len;
    OSC_MEMSET(block + block_len, 0, padsize);
    hashBlock();
    block_len = 0;
  }

  /* Now we pad out the current block to be exactly 56 bytes
     long, and add the data length. */
  padsize = 64 - block_len - 8;
  OSC_MEMSET(block + block_len, 0, padsize);
  m_c.context.w[14] = 0;
  m_c.context.w[15] = htonl(message_len_in_bits);
  hashBlock();

  for (int i = 0; i < 5; i++)
  {
    m_c.context.h[i] = ntohl(m_c.context.h[i]);
  }

  OSC_MEMMOVE(hash, m_c.context.h, hashLength);
}

void
osc::Sha1Hasher::hashBlock()
{
  DEBUG_STACK_FRAME;
  register u32 a = m_c.context.h[0];
  register u32 b = m_c.context.h[1];
  register u32 c = m_c.context.h[2];
  register u32 d = m_c.context.h[3];
  register u32 e = m_c.context.h[4];

#ifdef OPTIMIZE_SIZE
  int t;
  for (t = 0; t < 16; t++)
  {
    m_c.context.w[t] = ntohl(m_c.context.w[t]);
  }

  register osc::u32 temp;
  for (t = 0; t < 16; t++)
  {
    temp = ROTATE(5,a) + F0_19(b,c,d) + e + m_c.context.w[t&0xf] + K0_19;
    e=d; d=c; c=ROTATE(30,b); b=a; a=temp;
  }
  for (; t < 20; t++)
  {
    m_c.context.w[t&0xf] = ROTATE(1, m_c.context.w[(t+13)&0xf] ^
                                     m_c.context.w[(t+8)&0xf] ^
                                     m_c.context.w[(t+2)&0xf] ^
                                     m_c.context.w[t&0xf]);
    temp = ROTATE(5,a) + F0_19(b,c,d) + e + m_c.context.w[t&0xf] + K0_19;
    e=d; d=c; c=ROTATE(30,b); b=a; a=temp;
  }
  for (; t < 40; t++)
  {
    m_c.context.w[t&0xf] = ROTATE(1, m_c.context.w[(t+13)&0xf] ^
                                     m_c.context.w[(t+8)&0xf] ^
                                     m_c.context.w[(t+2)&0xf] ^
                                     m_c.context.w[t&0xf]);
    temp = ROTATE(5,a) + F20_39(b,c,d) + e + m_c.context.w[t&0xf] + K20_39;
    e=d; d=c; c=ROTATE(30,b); b=a; a=temp;
  }
  for (; t < 60; t++)
  {
    m_c.context.w[t&0xf] = ROTATE(1, m_c.context.w[(t+13)&0xf] ^
                                     m_c.context.w[(t+8)&0xf] ^
                                     m_c.context.w[(t+2)&0xf] ^
                                     m_c.context.w[t&0xf]);
    temp = ROTATE(5,a) + F40_59(b,c,d) + e + m_c.context.w[t&0xf] + K40_59;
    e=d; d=c; c=ROTATE(30,b); b=a; a=temp;
  }
  for (; t < 80; t++)
  {
    m_c.context.w[t&0xf] = ROTATE(1, m_c.context.w[(t+13)&0xf] ^
                                     m_c.context.w[(t+8)&0xf] ^
                                     m_c.context.w[(t+2)&0xf] ^
                                     m_c.context.w[t&0xf]);
    temp = ROTATE(5,a) + F60_79(b,c,d) + e + m_c.context.w[t&0xf] + K60_79;
    e=d; d=c; c=ROTATE(30,b); b=a; a=temp;
  }
#else
#  ifdef RISC_OPTIMIZED
#    define W(x) w##x
  osc::u32 w0 = ntohl(m_c.context.w[0]);
  osc::u32 w1 = ntohl(m_c.context.w[1]);
  osc::u32 w2 = ntohl(m_c.context.w[2]);
  osc::u32 w3 = ntohl(m_c.context.w[3]);
  osc::u32 w4 = ntohl(m_c.context.w[4]);
  osc::u32 w5 = ntohl(m_c.context.w[5]);
  osc::u32 w6 = ntohl(m_c.context.w[6]);
  osc::u32 w7 = ntohl(m_c.context.w[7]);
  osc::u32 w8 = ntohl(m_c.context.w[8]);
  osc::u32 w9 = ntohl(m_c.context.w[9]);
  osc::u32 w10 = ntohl(m_c.context.w[10]);
  osc::u32 w11 = ntohl(m_c.context.w[11]);
  osc::u32 w12 = ntohl(m_c.context.w[12]);
  osc::u32 w13 = ntohl(m_c.context.w[13]);
  osc::u32 w14 = ntohl(m_c.context.w[14]);
  osc::u32 w15 = ntohl(m_c.context.w[15]);
#  else
#    define W(x) m_c.context.w[x]
  int t;
  for (t = 0; t < 16; t++)
  {
    m_c.context.w[t] = ntohl(m_c.context.w[t]);
  }
#  endif
  PASS_0_15(a,b,c,d,e,W(0));
  PASS_0_15(e,a,b,c,d,W(1));
  PASS_0_15(d,e,a,b,c,W(2));
  PASS_0_15(c,d,e,a,b,W(3));
  PASS_0_15(b,c,d,e,a,W(4));
  PASS_0_15(a,b,c,d,e,W(5));
  PASS_0_15(e,a,b,c,d,W(6));
  PASS_0_15(d,e,a,b,c,W(7));
  PASS_0_15(c,d,e,a,b,W(8));
  PASS_0_15(b,c,d,e,a,W(9));
  PASS_0_15(a,b,c,d,e,W(10));
  PASS_0_15(e,a,b,c,d,W(11));
  PASS_0_15(d,e,a,b,c,W(12));
  PASS_0_15(c,d,e,a,b,W(13));
  PASS_0_15(b,c,d,e,a,W(14));
  PASS_0_15(a,b,c,d,e,W(15));
  PASS_16_19(e,a,b,c,d,W(0),W(13),W(8),W(2));
  PASS_16_19(d,e,a,b,c,W(1),W(14),W(9),W(3));
  PASS_16_19(c,d,e,a,b,W(2),W(15),W(10),W(4));
  PASS_16_19(b,c,d,e,a,W(3),W(0),W(11),W(5));
  PASS_20_39(a,b,c,d,e,W(4),W(1),W(12),W(6));
  PASS_20_39(e,a,b,c,d,W(5),W(2),W(13),W(7));
  PASS_20_39(d,e,a,b,c,W(6),W(3),W(14),W(8));
  PASS_20_39(c,d,e,a,b,W(7),W(4),W(15),W(9));
  PASS_20_39(b,c,d,e,a,W(8),W(5),W(0),W(10));
  PASS_20_39(a,b,c,d,e,W(9),W(6),W(1),W(11));
  PASS_20_39(e,a,b,c,d,W(10),W(7),W(2),W(12));
  PASS_20_39(d,e,a,b,c,W(11),W(8),W(3),W(13));
  PASS_20_39(c,d,e,a,b,W(12),W(9),W(4),W(14));
  PASS_20_39(b,c,d,e,a,W(13),W(10),W(5),W(15));
  PASS_20_39(a,b,c,d,e,W(14),W(11),W(6),W(0));
  PASS_20_39(e,a,b,c,d,W(15),W(12),W(7),W(1));
  PASS_20_39(d,e,a,b,c,W(0),W(13),W(8),W(2));
  PASS_20_39(c,d,e,a,b,W(1),W(14),W(9),W(3));
  PASS_20_39(b,c,d,e,a,W(2),W(15),W(10),W(4));
  PASS_20_39(a,b,c,d,e,W(3),W(0),W(11),W(5));
  PASS_20_39(e,a,b,c,d,W(4),W(1),W(12),W(6));
  PASS_20_39(d,e,a,b,c,W(5),W(2),W(13),W(7));
  PASS_20_39(c,d,e,a,b,W(6),W(3),W(14),W(8));
  PASS_20_39(b,c,d,e,a,W(7),W(4),W(15),W(9));
  PASS_40_59(a,b,c,d,e,W(8),W(5),W(0),W(10));
  PASS_40_59(e,a,b,c,d,W(9),W(6),W(1),W(11));
  PASS_40_59(d,e,a,b,c,W(10),W(7),W(2),W(12));
  PASS_40_59(c,d,e,a,b,W(11),W(8),W(3),W(13));
  PASS_40_59(b,c,d,e,a,W(12),W(9),W(4),W(14));
  PASS_40_59(a,b,c,d,e,W(13),W(10),W(5),W(15));
  PASS_40_59(e,a,b,c,d,W(14),W(11),W(6),W(0));
  PASS_40_59(d,e,a,b,c,W(15),W(12),W(7),W(1));
  PASS_40_59(c,d,e,a,b,W(0),W(13),W(8),W(2));
  PASS_40_59(b,c,d,e,a,W(1),W(14),W(9),W(3));
  PASS_40_59(a,b,c,d,e,W(2),W(15),W(10),W(4));
  PASS_40_59(e,a,b,c,d,W(3),W(0),W(11),W(5));
  PASS_40_59(d,e,a,b,c,W(4),W(1),W(12),W(6));
  PASS_40_59(c,d,e,a,b,W(5),W(2),W(13),W(7));
  PASS_40_59(b,c,d,e,a,W(6),W(3),W(14),W(8));
  PASS_40_59(a,b,c,d,e,W(7),W(4),W(15),W(9));
  PASS_40_59(e,a,b,c,d,W(8),W(5),W(0),W(10));
  PASS_40_59(d,e,a,b,c,W(9),W(6),W(1),W(11));
  PASS_40_59(c,d,e,a,b,W(10),W(7),W(2),W(12));
  PASS_40_59(b,c,d,e,a,W(11),W(8),W(3),W(13));
  PASS_60_79(a,b,c,d,e,W(12),W(9),W(4),W(14));
  PASS_60_79(e,a,b,c,d,W(13),W(10),W(5),W(15));
  PASS_60_79(d,e,a,b,c,W(14),W(11),W(6),W(0));
  PASS_60_79(c,d,e,a,b,W(15),W(12),W(7),W(1));
  PASS_60_79(b,c,d,e,a,W(0),W(13),W(8),W(2));
  PASS_60_79(a,b,c,d,e,W(1),W(14),W(9),W(3));
  PASS_60_79(e,a,b,c,d,W(2),W(15),W(10),W(4));
  PASS_60_79(d,e,a,b,c,W(3),W(0),W(11),W(5));
  PASS_60_79(c,d,e,a,b,W(4),W(1),W(12),W(6));
  PASS_60_79(b,c,d,e,a,W(5),W(2),W(13),W(7));
  PASS_60_79(a,b,c,d,e,W(6),W(3),W(14),W(8));
  PASS_60_79(e,a,b,c,d,W(7),W(4),W(15),W(9));
  PASS_60_79(d,e,a,b,c,W(8),W(5),W(0),W(10));
  PASS_60_79(c,d,e,a,b,W(9),W(6),W(1),W(11));
  PASS_60_79(b,c,d,e,a,W(10),W(7),W(2),W(12));
  PASS_60_79(a,b,c,d,e,W(11),W(8),W(3),W(13));
  PASS_60_79(e,a,b,c,d,W(12),W(9),W(4),W(14));
  PASS_60_79(d,e,a,b,c,W(13),W(10),W(5),W(15));
  PASS_60_79(c,d,e,a,b,W(14),W(11),W(6),W(0));
  PASS_60_79(b,c,d,e,a,W(15),W(12),W(7),W(1));
#endif

  m_c.context.h[0] += a;
  m_c.context.h[1] += b;
  m_c.context.h[2] += c;
  m_c.context.h[3] += d;
  m_c.context.h[4] += e;
}
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1300)
bool osc::operator < (osc::sha1_t const &digest1,
                      osc::sha1_t const &digest2)
{
  osc::count_t pos = 0;
  while(pos<20)
  {
    pos++;
    if(digest1.digest[pos] != digest2.digest[pos])
    {
      return digest1.digest[pos] < digest2.digest[pos];
    }

  }
  return false;
}

bool osc::operator == (osc::sha1_t const &digest1,
                       osc::sha1_t const &digest2)
{
  osc::count_t pos = 0;
  while(pos<20)
  {
    if(digest1.digest[pos] != digest2.digest[pos])
    {
      return false;
    }
    pos++;
  }
  return true;
}
#endif
