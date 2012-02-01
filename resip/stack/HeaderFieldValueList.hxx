#if !defined(RESIP_HEADERFIELDVALUELIST_HXX)
#define RESIP_HEADERFIELDVALUELIST_HXX 

#include <iosfwd>
#include <vector>

#include "rutil/StlPoolAllocator.hxx"
#include "rutil/PoolBase.hxx"

namespace resip
{

class Data;
class ParserContainerBase;
class HeaderFieldValue;

/**
   @internal
*/
class HeaderFieldValueList
{
   public:
      static const HeaderFieldValueList Empty;

      HeaderFieldValueList()
         : mHeaders(), 
           mPool(0),
           mParserContainer(0)
      {}

      HeaderFieldValueList(PoolBase& pool)
         : mHeaders(StlPoolAllocator<HeaderFieldValue, PoolBase>(&pool)),
           mPool(&pool),
           mParserContainer(0)
      {}

      ~HeaderFieldValueList();
      HeaderFieldValueList(const HeaderFieldValueList& rhs);
      HeaderFieldValueList(const HeaderFieldValueList& rhs, PoolBase& pool);
      HeaderFieldValueList& operator=(const HeaderFieldValueList& rhs);
      
      inline void setParserContainer(ParserContainerBase* parser) {mParserContainer = parser;}
      inline ParserContainerBase* getParserContainer() const {return mParserContainer;}

      EncodeStream& encode(int headerEnum, EncodeStream& str) const;
      EncodeStream& encode(const Data& headerName, EncodeStream& str) const;
      EncodeStream& encodeEmbedded(const Data& headerName, EncodeStream& str) const;

      bool empty() const {return mHeaders.empty();}
      size_t size() const {return mHeaders.size();}
      void clear();
      //void push_front(HeaderFieldValue* header) {mHeaders.push_front(header);}

      /**
         READ THIS CAREFULLY BEFORE USING THIS FUNCTION
         @param own Specifies whether the created HeaderFieldValue will take 
            ownership of the buffer passed. This will never make a copy 
            of the buffer; if own==false, the HeaderFieldValue will retain the 
            same reference that it would if own==true. The only difference is 
            that if own==false, the buffer will not be deleted when the 
            HeaderFieldValue goes away/releases its reference, while if 
            own==true the buffer will be deleted. This means that no matter what 
            you pass for this param, you must ensure that the buffer is not 
            deleted during the lifetime of this HeaderFieldValueList.
      */
      void push_back(const char* buffer, size_t length, bool own) 
      {
         mHeaders.push_back(HeaderFieldValue::Empty); 
         mHeaders.back().init(buffer,length,own);
      }

      //void pop_front() {mHeaders.pop_front();}
      void pop_back() {mHeaders.pop_back();};
      HeaderFieldValue* front() {return &mHeaders.front();}
      HeaderFieldValue* back() {return &mHeaders.back();}
      const HeaderFieldValue* front() const {return &mHeaders.front();}
      const HeaderFieldValue* back() const {return &mHeaders.back();}

      inline void reserve(size_t size)
      {
         mHeaders.reserve(size);
      }

      bool parsedEmpty() const;
   private:
      typedef std::vector<HeaderFieldValue, StlPoolAllocator<HeaderFieldValue, PoolBase > >  ListImpl;
   public:
      typedef ListImpl::iterator iterator;
      typedef ListImpl::const_iterator const_iterator;

      iterator begin() {return mHeaders.begin();}
      iterator end() {return mHeaders.end();}
      const_iterator begin() const {return mHeaders.begin();}
      const_iterator end() const {return mHeaders.end();}

   private:
      ListImpl mHeaders;
      PoolBase* mPool;
      ParserContainerBase* mParserContainer;

      void freeParserContainer();
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
