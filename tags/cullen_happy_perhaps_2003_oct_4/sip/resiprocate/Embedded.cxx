#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif


#include <cassert>

#include "resiprocate/Embedded.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/Symbols.hxx"

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
   while (get != end)
   {
      if (*get == Symbols::PERCENT[0])
      {
         *put = fromHex(*(get+1), *(get+2));
         get += 3;
         assert(get <= end);
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

Data
Embedded::encode(const Data& dat)
{
   Data out((int)(dat.size()*1.1), true);
   
   {
      DataStream str(out);
      for (Data::size_type i = 0; i < dat.size(); i++)
      {
         switch (dat[i])
         {
            case ';' :
            case '@' :
            case '&' :
            case '=' :
            case ' ' :
            case ',' :
            case '%' : 
            {
               str << Symbols::PERCENT;

               unsigned char temp = dat[i];
               int hi = (temp & 0xf0)>>4;
               int low = (temp & 0xf);

               str << hexMap[hi];
               str << hexMap[low];
               break;
            }
            default :
               str << dat[i];
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
