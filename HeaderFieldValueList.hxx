#ifndef HeaderFieldValueList_hxx
#define HeaderFieldValueList_hxx

#include <iostream>
#include <list>

namespace resip
{

class ParserContainerBase;
class HeaderFieldValue;

class HeaderFieldValueList
{
   public:
      HeaderFieldValueList()
         : mHeaders(), 
           mParserContainer(0)
      {}

      ~HeaderFieldValueList();
      HeaderFieldValueList(const HeaderFieldValueList& rhs);

      void setParserContainer(ParserContainerBase* parser) {mParserContainer = parser;}
      ParserContainerBase* getParserContainer() const {return mParserContainer;}

      std::ostream& encode(const Data& headerName, std::ostream& str);
      std::ostream& encodeEmbedded(const Data& headerName, std::ostream& str);

      bool empty() const {return mHeaders.empty();}
      size_t size() const {return mHeaders.size();}
      void push_front(HeaderFieldValue* header) {mHeaders.push_front(header);}
      void push_back(HeaderFieldValue* header) {mHeaders.push_back(header);}
      void pop_front() {mHeaders.pop_front();}
      void pop_back() {mHeaders.pop_back();};
      HeaderFieldValue* front() {return mHeaders.front();}
      HeaderFieldValue* back() {return mHeaders.back();}
      const HeaderFieldValue* front() const {return mHeaders.front();}
      const HeaderFieldValue* back() const {return mHeaders.back();}

   private:
      typedef std::list<HeaderFieldValue*> ListImpl;
   public:
      typedef ListImpl::iterator iterator;
      typedef ListImpl::const_iterator const_iterator;

      iterator begin() {return mHeaders.begin();}
      iterator end() {return mHeaders.end();}
      const_iterator begin() const {return mHeaders.begin();}
      const_iterator end() const {return mHeaders.end();}

   private:
      std::list<HeaderFieldValue*> mHeaders;
      ParserContainerBase* mParserContainer;
};

}

#endif


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
