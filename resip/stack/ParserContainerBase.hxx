#ifndef RESIP_ParserContainerBase_hxx
#define RESIP_ParserContainerBase_hxx

#include "resip/stack/ParserCategory.hxx"
#include <iosfwd>
#include "resip/stack/HeaderTypes.hxx"
#include <vector>

#include "rutil/StlPoolAllocator.hxx"
#include "rutil/PoolBase.hxx"

namespace resip
{

class HeaderFieldValueList;
class PoolBase;

/**
  @class ParserContainerBase
  @brief Abstract Base class implemented in derived class ParserContainer
  */
class ParserContainerBase
{
   public:
      typedef size_t size_type;

      /**
        @brief constructor; sets the type only
        */
      ParserContainerBase(Headers::Type type = Headers::UNKNOWN);

      /**
        @brief constructor; sets the type only
        */
      ParserContainerBase(Headers::Type type,
                           PoolBase& pool);

      /**
        @brief copy constructor copies the mType and the mParsers from the rhs
        @note this is a shallow copy
        */
      ParserContainerBase(const ParserContainerBase& rhs);

      ParserContainerBase(const ParserContainerBase& rhs,
                           PoolBase& pool);

      /**
        @brief assignment operator copies the mParsers from the rhs
        @note this is a shallow copy
        */
      ParserContainerBase& operator=(const ParserContainerBase& rhs);

      /**
        @brief virtual destructor - empty in this class
        */
      virtual ~ParserContainerBase();

      /**
        @brief clear the mParsers vector
        */
      inline void clear() {freeParsers(); mParsers.clear();}

      /**
        @brief pure virtual function to be implemented in derived classes 
         with the intention of cloning this object
        */
      virtual ParserContainerBase* clone() const = 0;

      /**
        @brief return the size of the mParsers vector
        */
      inline size_t size() const {return mParsers.size();}

      /**
        @brief return true or false indicating whether the mParsers vector is
         empty or not
        */
      inline bool empty() const {return mParsers.empty();}

      /**
        @internal 
        @brief the actual mechanics of parsing
        @todo add support for headers that are allowed to be empty like 
         Supported, Accept-Encoding, Allow-Events, Allow, Accept, 
         Accept-Language
        */
      EncodeStream& encode(const Data& headerName, EncodeStream& str) const;

      /**
        @internal
        @brief the actual mechanics of parsing
        */
      std::ostream& encode(Headers::Type type,std::ostream& str) const;

      /**
        @internal
        @brief the actual mechanics of parsing
        */
      EncodeStream& encodeEmbedded(const Data& headerName, EncodeStream& str) const;

      /**
        @brief if mParsers vector is not empty, erase the first element
        */
      void pop_front();

      /**
        @brief if mParsers vector is not empty, erase the first element
        */
      void pop_back();

      /**
        @brief append the vector to the mParsers vector held locally
        @param rhs is the vector whose elements will be added to the 
         local mParsers vector
        */
      void append(const ParserContainerBase& rhs);

      /**
        @brief pure virtual function to be implemented in derived classes
         The intention is to provide an ability to parse all elements 
         in the mParsers vector.
        */
      virtual void parseAll()=0;
   protected:
      const Headers::Type mType;

      /**
         @internal
      */
      class HeaderKit
      {
         public:
            static const HeaderKit Empty;

            HeaderKit(): pc(0){}

            // Poor man's move c'tor, watch out!
            HeaderKit(const HeaderKit& orig) 
            : pc(orig.pc),
               hfv(orig.hfv)
            {
               HeaderKit& nc_orig = const_cast<HeaderKit&>(orig);
               std::swap(nc_orig.pc, pc);
               hfv.swap(nc_orig.hfv);
            }
 
            // Poor man's move semantics, watch out!
            HeaderKit& operator=(const HeaderKit& rhs)
            {
               if(this!=&rhs)
               {
                  HeaderKit& nc_orig = const_cast<HeaderKit&>(rhs);
                  std::swap(nc_orig.pc, pc);
                  hfv.swap(nc_orig.hfv);
               }
               return *this;
            }
            
            ~HeaderKit()
            {}
            
            EncodeStream& encode(EncodeStream& str) const
            {
               if(pc)
               {
                  pc->encode(str);
               }
               else
               {
                  hfv.encode(str);
               }
               return str;
            }
            
            ParserCategory* pc;
            HeaderFieldValue hfv;
      };

      typedef std::vector<HeaderKit, StlPoolAllocator<HeaderKit, PoolBase> > Parsers;
      /**
        @brief the actual list (vector) of parsers on which encoding is done
        */
      Parsers mParsers;
      PoolBase* mPool;
      
      /**
        @brief copy header kits
        */
      void copyParsers(const Parsers& parsers);

      /**
        @brief free parser containers
        */
      void freeParsers();
      
      inline void freeParser(HeaderKit& kit)
      {
         if(kit.pc)
         {
            kit.pc->~ParserCategory();
            if(mPool)
            {
               mPool->deallocate(kit.pc);
            }
            else
            {
               ::operator delete(kit.pc);
            }
            kit.pc=0;
         }
      }

      inline ParserCategory* makeParser(const ParserCategory& orig)
      {
         return orig.clone(mPool);
      }
};
 
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005
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
