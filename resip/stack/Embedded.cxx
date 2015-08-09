#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif


#include "rutil/ResipAssert.h"

#include "resip/stack/Embedded.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

char fromHex(char h1, char h2)
{
   h1 = toupper(h1);
   h2 = toupper(h2);

   int i1;
   int i2;

   if (isdigit(h1))
   {
      i1 = h1 - '0';
   }
   else
   {
      i1 = h1 - 'A' + 10;
   }

   if (isdigit(h2))
   {
      i2 = h2 - '0';
   }
   else
   {
      i2 = h2 - 'A' + 10;
   }
   
   return i1*16+i2;
}

char*
Embedded::decode(const Data& in, unsigned int& count)
{
   const char *get = in.data();
   const char *end = get + in.size();
   char *ret = new char[in.size()];
   char *put = ret;

   count = 0;
   while (get < end)
   {
      if (*get == Symbols::PERCENT[0] && get+2 < end)
      {
         *put = fromHex(*(get+1), *(get+2));
         get += 3;
      }
      else
      {
         *put = *get;
         get++;
      }
      count++;
      put++;
   }

   return ret;
}

static char hexMap[] = "0123456789ABCDEF";

/*
  This method encodes the hname and hvalue production of SIP-URI.

    SIP-URI          =  "sip:" [ userinfo ] hostport
                        uri-parameters [ headers ]

    headers         =  "?" header *( "&" header )
    header          =  hname "=" hvalue
    hname           =  1*( hnv-unreserved / unreserved / escaped )
    hvalue          =  *( hnv-unreserved / unreserved / escaped )

    hnv-unreserved  =  "[" / "]" / "/" / "?" / ":" / "+" / "$"

    unreserved  =  alphanum / mark
    mark        =  "-" / "_" / "." / "!" / "~" / "*" / "'"
                   / "(" / ")"
    escaped     =  "%" HEXDIG HEXDIG

    alphanum  =  ALPHA / DIGIT

  It is both unsafe and unwise to express what needs to be escaped
  and simply not escape everything else, because the omission of an item
  in need of escaping will cause mis-parsing on the remote end.
  Because escaping unnecessarily causes absolutely no harm, the omission
  of a symbol from the list of items positively allowed causes no
  damage whatsoever.
*/

Data
Embedded::encode(const Data& dat)
{
   Data out((int)((dat.size()*11)/10), Data::Preallocate);
   
   {
      DataStream str(out);
      for (Data::size_type i = 0; i < dat.size(); i++)
      {
         switch (dat[i])
         {
            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9': case 'a': case 'b':
            case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
            case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
            case 'o': case 'p': case 'q': case 'r': case 's': case 't':
            case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
            case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
            case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
            case 'Y': case 'Z': case '[': case ']': case ',': case '?':
            case ':': case '+': case '$': case '-': case '_': case '.':
            case '!': case '~': case '*': case '\'': case '(': case ')':
               str << dat[i];
               break;

            default:
            {
               str << Symbols::PERCENT;

               unsigned char temp = dat[i];
               int hi = (temp & 0xf0)>>4;
               int low = (temp & 0xf);

               str << hexMap[hi];
               str << hexMap[low];
            }
         }
      }
   }

   return out;
}
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
