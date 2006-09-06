// ==========================================================================================================
// InternalSendPagerMessage.hxx                                                          2006 @ TelTel
// ==========================================================================================================
// Post to send asynchronized client pager message.
// ==========================================================================================================
#ifndef RESIP_InternalSendPagerMessage_hxx
#define RESIP_InternalSendPagerMessage_hxx

#include <memory>

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/dum/ClientPagerMessage.hxx"

namespace resip
{

   class DUM_API InternalSendPagerMessage : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalSendPagerMessage);
      InternalSendPagerMessage(ClientPagerMessageHandle& h,
                               std::auto_ptr<Contents>   contents,
                               std::auto_ptr< std::map<resip::Data, resip::Data> > extraHeaders) 
         : mClientPager(h)
          ,mContents(contents)
          ,mExtraHeaders(extraHeaders) {/*Empty*/}

      virtual Message* clone() const {assert(false); return NULL;}
      virtual std::ostream& encode(std::ostream& strm) const { return encodeBrief(strm); }
      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalSendPagerMessage"; }

      virtual void execute()
      {
         if(mClientPager.isValid())
         {
            mClientPager->page(mContents, mExtraHeaders);
         }
      }

      ClientPagerMessageHandle                            mClientPager;
      std::auto_ptr<Contents>                             mContents;
      std::auto_ptr< std::map<resip::Data, resip::Data> > mExtraHeaders;
   };

}

#endif // RESIP_InternalSendPagerMessage_hxx

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

