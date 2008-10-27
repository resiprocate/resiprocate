#include "resipfaststreams.hxx"

#include <math.h>

#ifndef RESIP_USE_STL_STREAMS
resip::ResipStdCOStream resip::resipFastCerr(resip::ResipStdBuf::stdCerr);
resip::ResipStdCOStream resip::resipFastCout(resip::ResipStdBuf::stdCout);
#endif
resip::ResipStdCOStream resip::resipFastNull(resip::ResipStdBuf::null);

// other interesting references on num to string convesion
// http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
// and http://www.ddj.com/dept/cpp/184401596?pgno=6

// Version 19-Nov-2007
// Fixed round-to-even rules to match printf
//   thanks to Johannes Otepka

/**
 * Powers of 10
 * 10^0 to 10^9
 */
static const double pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000,
                               10000000, 100000000, 1000000000};

static void strreverse(char* begin, char* end)
{
    char aux;
    while (end > begin)
        aux = *end, *end-- = *begin, *begin++ = aux;
}

void modp_itoa10(int value, char* str)
{
    char* wstr=str;
    int sign;
    // Take care of sign
    if ((sign=value) < 0) value = -value;
    // Conversion. Number is reversed.
    do *wstr++ = (48 + (value % 10)); while( value /= 10);
    if (sign < 0) *wstr++ = '-';
    *wstr='\0';

    // Reverse string
    strreverse(str,wstr-1);
}

void modp_uitoa10(unsigned int value, char* str)
{
    char* wstr=str;
    // Conversion. Number is reversed.
    do *wstr++ = 48 + (value % 10); while (value /= 10);
    *wstr='\0';
    // Reverse string
    strreverse(str, wstr-1);
}

void modp_itoa10_64(__int64 value, char* str)
{
    char* wstr=str;
    __int64 sign;
    // Take care of sign
    if ((sign=value) < 0) value = -value;
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (value % 10)); while( value /= 10);
    if (sign < 0) *wstr++ = '-';
    *wstr='\0';

    // Reverse string
    strreverse(str,wstr-1);
}

void modp_uitoa10_64(UInt64 value, char* str)
{
    char* wstr=str;
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (value % 10)); while (value /= 10);
    *wstr='\0';
    // Reverse string
    strreverse(str, wstr-1);
}

void modp_dtoa(double value, char* str, int prec)
{
    /* if input is larger than thres_max, revert to exponential */
    const double thres_max = (double)(0x7FFFFFFF);

    double diff = 0.0;
    char* wstr = str;

    if (prec < 0) {
        prec = 0;
    } else if (prec > 9) {
        /* precision of >= 10 can lead to overflow errors */
        prec = 9;
    }


    /* we'll work in positive values and deal with the
       negative sign issue later */
    int neg = 0;
    if (value < 0) {
        neg = 1;
        value = -value;
    }


    int whole = (int) value;
    double tmp = (value - whole) * pow10[prec];
    unsigned int frac = (unsigned int)(tmp);
    diff = tmp - frac;

    if (diff > 0.5) {
        ++frac;
        /* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
        if (frac >= pow10[prec]) {
            frac = 0;
            ++whole;
        }
    } else if (diff == 0.5 && ((frac == 0) || (frac & 1))) {
        /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
        ++frac;
    }

    /* for very large numbers switch back to native sprintf for exponentials.
       anyone want to write code to replace this? */
    /*
       normal printf behavior is to print EVERY whole number digit
       which can be 100s of characters overflowing your buffers == bad
    */
    if (value > thres_max) {
        sprintf(str, "%e", neg ? -value : value);
        return;
    }

    if (prec == 0) {
        diff = value - whole;
        if (diff > 0.5) {
            /* greater than 0.5, round up, e.g. 1.6 -> 2 */
            ++whole;
        } else if (diff == 0.5 && (whole & 1)) {
            /* exactly 0.5 and ODD, then round up */
            /* 1.5 -> 2, but 2.5 -> 2 */
            ++whole;
        }
    } else {
        int count = prec;
        // now do fractional part, as an unsigned number
        do {
            --count;
            *wstr++ = 48 + (frac % 10);
        } while (frac /= 10);
        // add extra 0s
        while (count-- > 0) *wstr++ = '0';
        // add decimal
        *wstr++ = '.';
    }

    // do whole part
    // Take care of sign
    // Conversion. Number is reversed.
    do *wstr++ = 48 + (whole % 10); while (whole /= 10);
    if (neg) {
        *wstr++ = '-';
    }
    *wstr='\0';
    strreverse(str, wstr-1);
}

static const int resipInt32MaxSize=12;
static const int resipUInt32MaxSize=11;
static const int resipDoubleMaxPrecision=11;
static const int resipInt64MaxSize=21;
static const int resipUInt64MaxSize=20;

unsigned int resip_itoa10(int val, char* buf, unsigned int bufSize)
{
   if (!buf || (bufSize < (resipInt32MaxSize+1)))
   {
      assert(0);
      return 0;
   }

   if (val == 0)
   {
      buf[0] = '0';
      buf[1] = 0;
      return 1;
   }

   bool neg = false;
   
   int value = val;
   if (value < 0)
   {
      value = -value;
      neg = true;
   }

   int c = 0;
   int v = value;
   while (v /= 10)
   {
      ++c;
   }

   if (neg)
   {
      ++c;
   }

   unsigned size = c+1;
   buf[c+1] = 0;
   
   v = value;
   while (v)
   {
      buf[c--] = '0' + v%10;
      v /= 10;
   }

   if (neg)
   {
      buf[0] = '-';
   }

   return size;
}

unsigned int resip_uitoa10(unsigned int value, char* buf, unsigned int bufSize)
{
   if (!buf || (bufSize < (resipUInt32MaxSize+1)))
   {
      assert(0);
      return 0;
   }

   if (value == 0)
   {
      buf[0] = '0';
      buf[1] = 0;
      return 1;
   }

   int c = 0;
   unsigned long v = value;
   while (v /= 10)
   {
      ++c;
   }

   unsigned size = c+1;
   buf[c+1] = 0;
   
   v = value;
   while (v)
   {
      unsigned int digit = v%10;
      unsigned char d = (char)digit;
      buf[c--] = '0' + d;
      v /= 10;
   }

   return size;
}

unsigned int resip_dtoa(double value, char* buf, unsigned int bufSize, unsigned int precision)
{
   
   if (!buf || bufSize < (resipInt32MaxSize + precision + 2))
   {
      assert(0);
      return 0;
   }

   if (precision > resipDoubleMaxPrecision)
   {
      assert(0);
      precision = resipDoubleMaxPrecision;
   }

   double v = value;
   bool neg = (value < 0.0);
   
   if (neg)
   {
      v = -v;
   }

   char m[resipInt32MaxSize];
   unsigned int msize = resip_uitoa10((unsigned long)v,m,resipInt32MaxSize);

   // remainder
   v = v - floor(v);

   int p = precision;
   while (p--)
   {
      v *= 10;
   }

   int dec = (int)floor(v+0.5);
   char d[resipDoubleMaxPrecision+1];
   unsigned int dsize = 0;

   if (dec == 0)
   {
      d[0] = '0';
      d[1] = 0;
      dsize = 1;
   }
   else
   {
      d[precision] = 0;
      p = precision;
      // neglect trailing zeros
      bool significant = false;
      while (p--)
      {
         if (dec % 10 || significant)
         {
            significant = true;
            ++dsize;
            d[p] = '0' + (dec % 10);
         }
         else
         {
            d[p] = 0;
         }
         
         dec /= 10;
      }
   }

   if (neg)
   {
      buf[0] = '-';
      memcpy(buf+1, m, msize);
      buf[1+msize] = '.';
      memcpy(buf+1+msize+1, d, dsize+1);
      return msize + dsize + 2;
   }
   else
   {
      memcpy(buf, m, msize);
      buf[msize] = '.';
      memcpy(buf+msize+1, d, dsize+1);
      return msize + dsize + 1;
   }
}

unsigned int resip_itoa10_64(long long val, char* buf, unsigned int bufSize)
{
   if (!buf || (bufSize < (resipInt64MaxSize+1)))
   {
      assert(0);
      return 0;
   }

   if (val == 0)
   {
      buf[0] = '0';
      buf[1] = 0;
      return 1;
   }

   bool neg = false;
   
   long long value = val;
   if (value < 0)
   {
      value = -value;
      neg = true;
   }

   int c = 0;
   long long v = value;
   while (v /= 10)
   {
      ++c;
   }

   if (neg)
   {
      ++c;
   }

   unsigned int size = c+1;
   buf[c+1] = 0;
   
   v = value;
   while (v)
   {
      buf[c--] = '0' + (char)(v%10);
      v /= 10;
   }

   if (neg)
   {
      buf[0] = '-';
   }
   return size;
}

unsigned int resip_uitoa10_64(UInt64 value, char* buf, unsigned int bufSize)
{
   if (!buf || (bufSize < (resipUInt64MaxSize + 1)))
   {
      assert(0);
      return 0;
   }

   if (value == 0)
   {
      buf[0] = '0';
      buf[1] = 0;
      return 1;
   }

   int c = 0;
   UInt64 v = value;
   while (v /= 10)
   {
      ++c;
   }

   unsigned int size = c+1;
   buf[c+1] = 0;
   
   v = value;
   while (v)
   {
      UInt64 digit = v%10;
      unsigned char d = (char)digit;
      buf[c--] = '0' + d;
      v /= 10;
   }
   return size;
}
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2005.   All rights reserved.
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
