#include "resiprocate/ParseUtil.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/os/DnsUtil.hxx"

using namespace resip;

/*
IPv4address    =  1*3DIGIT "." 1*3DIGIT "." 1*3DIGIT "." 1*3DIGIT
IPv6reference  =  "[" IPv6address "]"
IPv6address    =  hexpart [ ":" IPv4address ]
hexpart        =  hexseq / hexseq "::" [ hexseq ] / "::" [ hexseq ]
hexseq         =  hex4 *( ":" hex4)
hex4           =  1*4HEXDIG
*/

// 1:1
//        FEDC:BA98:7654:3210:FEDC:BA98:7654:3210 
//        1080::8:800:200C:417A

// is this an IPV6 address?
bool 
ParseUtil::isIpV6Address(const Data& addr)
{
   return DnsUtil::isIpV6Address(addr);

   // ambiguity problem;
   // 1:1:1:1
   // 1:1:1:126.17.123.78
   //       ^ouch
#if 0   
   ParseBuffer pb(addr, "IPV6");

   if (!isxdigit(*pb.position()))
   {
      return false;
   }
   for (int i = 0; i < 3; i++)
   {
      if (*pb.position() == Symbols::COLON[0])
      {
	 pb.skipChar();
	 break;
      }

      if (!isxdigit(*pb.position()))
      {
	 return false;
      }
      pb.skipChar();
   }

   for (int h = 0; h < 5; h++)
   {
      for (int i = 0; i < 4; i++)
      {
	 if (*pb.position() == Symbols::COLON[0])
	 {
	    pb.skipChar();
	    break;
	 }

	 if (!isxdigit(*pb.position()))
	 {
	    return false;
	 }
	 pb.skipChar();
      }
   }

   // optional IPV4
   if (!pb.eof())
   {
      pb.skipChar(Symbols::COLON[0]);
      if (!isdigit(*pb.position()))
      {
	 return false;
      }
      for (int i = 0; i < 2; i++)
      {
	 if (*pb.position() == Symbols::DOT[0])
	 {
	    pb.skipChar();
	    break;
	 }

	 if (!isdigit(*pb.position()))
	 {
	    return false;
	 }
	 pb.skipChar();
      }
   }

   return pb.eof();
   return true;
#endif
}

//        1080:0000:8:800:200C:417A
//        1:::::
//        11:::::
//        111:::::
//        1111:::::
bool
ParseUtil::fastRejectIsIpV6Address(const Data& addr)
{
   if (addr.empty())
   {
      return false;
   }

   // first character must be a hex digit
   if (!isxdigit(*addr.data()))
   {
      return false;
   }

   switch (addr.size())
   {
      case 1:
         return false;
      case 2:
         return (*(addr.data()+2) == Symbols::COLON[0] ||
                 *(addr.data()+1) == Symbols::COLON[0]);
      case 3:
         return (*(addr.data()+3) == Symbols::COLON[0] ||
                 *(addr.data()+2) == Symbols::COLON[0] ||
                 *(addr.data()+1) == Symbols::COLON[0]);
      case 4:
         return (*(addr.data()+4) == Symbols::COLON[0] ||
                 *(addr.data()+3) == Symbols::COLON[0] ||
                 *(addr.data()+2) == Symbols::COLON[0] ||
                 *(addr.data()+1) == Symbols::COLON[0]);
      default:
         return (*(addr.data()+5) == Symbols::COLON[0] ||
                 *(addr.data()+4) == Symbols::COLON[0] ||
                 *(addr.data()+3) == Symbols::COLON[0] ||
                 *(addr.data()+2) == Symbols::COLON[0] ||
                 *(addr.data()+1) == Symbols::COLON[0]);
   }
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
