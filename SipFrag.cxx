#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/SipFrag.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"

#ifndef NEW_MSG_HEADER_SCANNER
#include "resiprocate/Preparse.hxx"
#else
#include "resiprocate/MsgHeaderScanner.hxx"
#endif

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

ContentsFactory<SipFrag> SipFrag::Factory;

SipFrag::SipFrag(const Mime& contentsType)
   : Contents(contentsType),
     mMessage(new SipMessage())
{}

SipFrag::SipFrag(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mMessage(0)
{
}

SipFrag::SipFrag(const SipFrag& rhs)
   : Contents(rhs),
     mMessage(rhs.mMessage ? new SipMessage(*rhs.mMessage) : 0)
{
}

SipFrag::~SipFrag()
{
   delete mMessage;
}

SipFrag&
SipFrag::operator=(const SipFrag& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      delete mMessage;
      if (rhs.mMessage)
      {
         mMessage = new SipMessage(*rhs.mMessage);
      }
      else
      {
         mMessage = 0;
      }
   }
   
   return *this;
}

Contents* 
SipFrag::clone() const
{
   return new SipFrag(*this);
}

const Mime& 
SipFrag::getStaticType() 
{
   static Mime type("message", "sipfrag");
   return type;
}

std::ostream& 
SipFrag::encodeParsed(std::ostream& str) const
{
   mMessage->encode(str);

   return str;
}

#ifndef NEW_MSG_HEADER_SCANNER // {

void 
SipFrag::parse(ParseBuffer& pb)
{
   DebugLog(<< "SipFrag::parse: " << pb.position());

   mMessage = new SipMessage();

   Preparse pre;

   pb.assertNotEof();
   const char* const_buffer = pb.position();
   char* buffer = const_cast<char*>(const_buffer);

   size_t size = pb.end() - pb.position();

   // !dlb! fragment not required to CRLF terminate
   // need another interface to preparse?
   // !ah! removed size check .. process() cannot process more
   // than size bytes of the message.
   if (pre.process(*mMessage, buffer, size))
   {
     pb.fail(__FILE__, __LINE__);
   }
   else 
   {
     
     size_t used =
         pre.nBytesUsed();

     // !ah! I think this is broken .. if we are UDP then the 
     // remainder is the SigFrag, not the Content-Length... ??
     if (
         pre.isHeadersComplete()
         && 
         mMessage->exists(h_ContentLength))
      {
         assert(used == pre.nDiscardOffset());
         mMessage->setBody( buffer+used, int(size-used) );
      }
      else
      {
        // !ah! So the headers weren't complete. Why are we here?
        // !dlb! 
         if (mMessage->exists(h_ContentLength))
         {
            pb.reset(buffer + used);
            pb.skipChars(Symbols::CRLF);
            mMessage->setBody(pb.position(),int(pb.end()-pb.position()) );
         }
      }
      pb.reset(pb.end());
   }
}

#else // defined(NEW_MSG_HEADER_SCANNER) } {

void 
SipFrag::parse(ParseBuffer& pb)
{
   DebugLog(<< "SipFrag::parse: " << pb.position());

   mMessage = new SipMessage();

   pb.assertNotEof();
   const char *constBuffer = pb.position();
   char *buffer = const_cast<char *>(constBuffer);

   size_t size = pb.end() - pb.position();

   // !ah! removed size check .. process() cannot process more
   // than size bytes of the message.

   MsgHeaderScanner msgHeaderScanner;
   msgHeaderScanner.prepareForMessage(mMessage);
   enum { sentinelLength = 4 };  // Two carriage return / line feed pairs.
   char saveTermCharArray[sentinelLength];
   char *termCharArray = buffer + size;
   saveTermCharArray[0] = termCharArray[0];
   saveTermCharArray[1] = termCharArray[1];
   saveTermCharArray[2] = termCharArray[2];
   saveTermCharArray[3] = termCharArray[3];
   termCharArray[0] = '\r';
   termCharArray[1] = '\n';
   termCharArray[2] = '\r';
   termCharArray[3] = '\n';
   char *scanTermCharPtr;
   MsgHeaderScanner::ScanChunkResult scanChunkResult =
       msgHeaderScanner.scanChunk(buffer,
                                  size + sentinelLength,
                                  &scanTermCharPtr);
   termCharArray[0] = saveTermCharArray[0];
   termCharArray[1] = saveTermCharArray[1];
   termCharArray[2] = saveTermCharArray[2];
   termCharArray[3] = saveTermCharArray[3];
   if (scanChunkResult != MsgHeaderScanner::scrEnd) {
     pb.fail(__FILE__, __LINE__);
   } else {
     size_t used = scanTermCharPtr - buffer;

     // !ah! I think this is broken .. if we are UDP then the 
     // remainder is the SigFrag, not the Content-Length... ??
     if (mMessage->exists(h_ContentLength))
      {
         mMessage->setBody(scanTermCharPtr,
                           static_cast<int>(size - used));
      }
      else
      {
        // !ah! So the headers weren't complete. Why are we here?
        // !dlb! 
         if (mMessage->exists(h_ContentLength))
         {
            pb.reset(buffer + used);
            pb.skipChars(Symbols::CRLF);
            mMessage->setBody(pb.position(),int(pb.end()-pb.position()) );
         }
      }
      pb.reset(pb.end());
   }
}

#endif // defined(NEW_MSG_HEADER_SCANNER) }
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
