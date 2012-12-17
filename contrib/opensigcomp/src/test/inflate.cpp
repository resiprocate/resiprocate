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

#include "BitBuffer.h"
#include "Types.h"
#include "inflate.h"
#include <string.h>
#include <iostream>

/*
  From RFC 1951:

  loop (until end of block code recognized)
     decode literal/length value from input stream
     if value < 256
        copy value (literal byte) to output stream
     otherwise
        if value = end of block (256)
           break from loop
        otherwise (value = 257..285)
           decode distance from input stream

           move backwards distance bytes in the output
           stream, and copy length bytes from this
           position to the output stream.
  end loop
*/

osc::u16 getLiteralOrLength(osc::BitBuffer &input);
osc::u16 getDistance(osc::BitBuffer &input);

size_t
inflate(osc::BitBuffer &input, osc::byte_t *output, size_t outputSize)
{
  osc::u16 value;
  osc::u16 length;
  osc::u16 distance;

  input.setPackOrder(osc::BitBuffer::MSB_FIRST);

  size_t curr = 0;

  while (input.getBufferSize() >= 7)
  {
    value = getLiteralOrLength(input);

    if (value == 0xFFFF)
    {
      return curr;
    }

    if (value < 256)
    {
      output[curr++] = value;
    }
    else if (value == 256)
    {
      std::cout << __FILE__ << ":" << __LINE__ 
                << ": Decoded value = 256" << std::endl;
      return curr;
    }
    else
    {
      length = value - 254;

      if (length > 10)
      {
        if (length <= 14)
        {
          // 1 extra bit
          length = (length - 11) * 2 + 11 + input.readMsbFirst(1);
        }
        else if (length <= 18)
        {
          // 2 extra bits
          length = (length - 15) * 4 + 19 + input.readMsbFirst(2);
        }
        else if (length <= 22)
        {
          // 3 extra bits
          length = (length - 19) * 8 + 35 + input.readMsbFirst(3);
        }
        else if (length <= 26)
        {
          // 4 extra bits
          length = (length - 23) * 16 + 67 + input.readMsbFirst(4);
        }
        else if (length <= 30)
        {
          // 5 extra bits
          length = (length - 27) * 32 + 131 + input.readMsbFirst(5);
        }
        else if (length == 31)
        {
          length = 258;
        }
      }

      if (input.getBufferSize() < 5)
      {
        return curr;
      }

      distance = getDistance(input);

      if (distance > curr)
      {
        return 0;
      }

      for (int i = 0; i < length; i++)
      {
        output[curr] = output[curr-distance];
        curr++;
      }
    }
  }

  return curr;
}

osc::u16
getLiteralOrLength(osc::BitBuffer &input)
{
  osc::u16 val;
  val = input.readMsbFirst(7);

  // Length
  if (val <= 23)
  {
    val = val + 256;
    return val;
  }

  if (input.getBufferSize() < 1)
  {
    return 0xFFFF;
  }
  
  val = (val << 1) | input.readMsbFirst(1);

  // Literal
  if (val >= 48 && val <= 191)
  {
    val = (val - 48);
    return val;
  }

  // Length
  if (val >= 192 && val <= 199)
  {
    val = val - 192 + 280;
    return val;
  }

  if (input.getBufferSize() < 1)
  {
    return 0xFFFF;
  }
  val = (val << 1) | input.readMsbFirst(1);

  // Literal
  val = (val - 400 + 144);
  return val;
}

osc::u16
getDistance(osc::BitBuffer &input)
{
  osc::u16 distance = input.readMsbFirst(5);

  switch (distance)
  {
    case 0: case 1: case 2: case 3:
      return (distance + 1);

    case 4: case 5:
      return ((distance - 4) * (1<<1)) + 5 + input.readMsbFirst(1);

    case 6: case 7:
      return ((distance - 6) * (1<<2)) + 9 + input.readMsbFirst(2);

    case 8: case 9:
      return ((distance - 8) * (1<<3)) + 17 + input.readMsbFirst(3);

    case 10: case 11:
      return ((distance - 10) * (1<<4)) + 33 + input.readMsbFirst(4);

    case 12: case 13:
      return ((distance - 12) * (1<<5)) + 65 + input.readMsbFirst(5);

    case 14: case 15:
      return ((distance - 14) * (1<<6)) + 129 + input.readMsbFirst(6);

    case 16: case 17:
      return ((distance - 16) * (1<<7)) + 257 + input.readMsbFirst(7);

    case 18: case 19:
      return ((distance - 18) * (1<<8)) + 513 + input.readMsbFirst(8);

    case 20: case 21:
      return ((distance - 20) * (1<<9)) + 1025 + input.readMsbFirst(9);

    case 22: case 23:
      return ((distance - 22) * (1<<10)) + 2049 + input.readMsbFirst(10);

    case 24: case 25:
      return ((distance - 24) * (1<<11)) + 4097 + input.readMsbFirst(11);

    case 26: case 27:
      return ((distance - 26) * (1<<12)) + 8193 + input.readMsbFirst(12);

    case 28: case 29:
      return ((distance - 28) * (1<<13)) + 16385 + input.readMsbFirst(13);
  }
  return 0;
}
