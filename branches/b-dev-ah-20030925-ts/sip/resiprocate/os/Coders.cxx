
#include <assert.h>

#include "compat.hxx"
#include "resiprocate/os/Coders.hxx"


namespace resip
{

using namespace std;

unsigned char Base64Coder::codeChar[] = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";



Data Base64Coder::encode(const Data& data)
{

   int srcLength = data.size();
   unsigned int dstLimitLength = srcLength*4/3 + 1;
   unsigned char * dstData = new unsigned char[dstLimitLength];
   unsigned int dstIndex = 0;

   const char * p = static_cast<const char *>( data.data() );

   for(int index=0;index<srcLength;index+=3)
   {
      unsigned char codeBits = (p[index] & 0xfc)>>2;
        
        assert(codeBits < 64);
        dstData[dstIndex++] = codeChar[codeBits]; // c0 output
        assert(dstIndex <= dstLimitLength);

        // do second codeBits
        codeBits = ((p[index]&0x3)<<4);
        if (index+1 < srcLength)
        {
            codeBits |= ((p[index+1]&0xf0)>>4);
        }
        assert(codeBits < 64);
        dstData[dstIndex++] = codeChar[codeBits]; // c1 output
        assert(dstIndex <= dstLimitLength);

        if (index+1 >= srcLength) break; // encoded d0 only
        

        // do third codeBits
        codeBits = ((p[index+1]&0xf)<<2);
        if (index+2 < srcLength)
        {
            codeBits |= ((p[index+2]&0xc0)>>6);
        }
        assert(codeBits < 64);
        dstData[dstIndex++] = codeChar[codeBits]; // c2 output
        assert(dstIndex <= dstLimitLength);

        if (index+2 >= srcLength) break; // encoded d0 d1 only
        
        // do fourth codeBits
        codeBits = ((p[index+2]&0x3f));
        assert(codeBits < 64);
        dstData[dstIndex++] = codeChar[codeBits]; // c3 output
        assert(dstIndex <= dstLimitLength);
        // outputed all d0,d1, and d2
    }
    return Data(dstData,dstIndex);
}

unsigned char Base64Coder::toBits(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return c-'A';
    if (c >= 'a' && c <= 'z') return c-'a'+26;
    if (c >= '0' && c <= '9') return c-'0'+52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return 0;
}

Data Base64Coder::decode(const Data& source)
{
   int srcLen = source.size();

   Data output((srcLen*3)/4,true);

   const char * p = static_cast<const char *>(source.data());

   for( int index = 0 ; index < srcLen ; index+=4)
   {
      if (index+1 >= srcLen) break;
      
      unsigned char c0 = toBits(p[index]);
      unsigned char c1 = toBits(p[index+1]);
        
      output += 
         (c0 << 2) | 
         ((c1 & 0x30) >> 4) ;
      
      // done d0
      
      if (index + 2 >= srcLen) break; // no data for d1
        
      unsigned char c2 = toBits(p[index+2]);
        
      output += 
         ((c1 & 0xf) << 4 ) | 
         ((c2 & 0x3c) >> 2);
        
      // done d1
      
      if (index + 3 >= srcLen) break; // no data for d2
      
      unsigned char c3 = toBits(p[index+3]);
      
      output += ((c2&0x3) << 6) | c3;
      // done d2
   }
   return output;
}

 
};


// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End
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
