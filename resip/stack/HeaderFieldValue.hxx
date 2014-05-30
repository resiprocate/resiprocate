#if !defined(RESIP_HEADERFIELDVALUE_HXX)
#define RESIP_HEADERFIELDVALUE_HXX 

#include "rutil/ParseException.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/ParameterTypes.hxx"

#include <iosfwd>

namespace resip
{

class ParserCategory;
class UnknownParameter;
class ParseBuffer;

/**
   @internal
*/
class HeaderFieldValue
{
   public:
      static const HeaderFieldValue Empty;
      
      enum CopyPaddingEnum
      {
         CopyPadding
      };

      enum NoOwnershipEnum
      {
         NoOwnership
      };

      HeaderFieldValue()
         : mField(0), //this must be initialized to 0 or ParserCategory will parse
           mFieldLength(0),
           mMine(false)
      {}
      HeaderFieldValue(const char* field, unsigned int fieldLength);
      HeaderFieldValue(const HeaderFieldValue& hfv);
      HeaderFieldValue(const HeaderFieldValue& hfv, CopyPaddingEnum);
      HeaderFieldValue(const HeaderFieldValue& hfv, NoOwnershipEnum);
      HeaderFieldValue& operator=(const HeaderFieldValue&);
      HeaderFieldValue& copyWithPadding(const HeaderFieldValue& rhs);
      HeaderFieldValue& swap(HeaderFieldValue& orig);

      ~HeaderFieldValue();

      EncodeStream& encode(EncodeStream& str) const;

      inline void init(const char* field, size_t length, bool own)
      {
         if(mMine)
         {
            delete [] mField;
         }
         
         mField=field;
         mFieldLength=(unsigned int)length;
         mMine=own;
      }
      
      inline const char* getBuffer() const {return mField;}
      inline unsigned int getLength() const {return mFieldLength;}
      inline void clear()
      {
         if (mMine)
         {
           delete[] mField;
           mMine=false;
         }
        mField=0;
        mFieldLength=0;
      }

      // const because Data::Share implies read-only access
      void toShareData(Data& data) const
      {
         data.setBuf(Data::Share, mField, mFieldLength);
      }

      // not const because Data::Borrow implies read/write access
      void toBorrowData(Data& data)
      {
         data.setBuf(Data::Borrow, mField, mFieldLength);
      }

   private:
      
      const char* mField;
      unsigned int mFieldLength;
      bool mMine;

      friend EncodeStream& operator<<(EncodeStream&, HeaderFieldValue&);
};

EncodeStream& operator<<(EncodeStream& stream, 
			 HeaderFieldValue& hList);


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
 * set shiftwidth=3 expandtab:
 */
