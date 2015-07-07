#if !defined(RESIP_PIDF_HXX)
#define RESIP_PIDF_HXX 

#include <vector>

#include "resip/stack/Contents.hxx"
#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "resip/stack/QValue.hxx"

namespace resip
{

/**
   @deprecated
   @brief Deprecated
   
   SIP body type for holding PIDF contents (MIME content-type application/pidf+xml).
*/
class Pidf : public Contents
{
   public:
      static const Pidf Empty;

      RESIP_HeapCount(Pidf);
      Pidf(const Mime& contentType);
      explicit Pidf(const Uri& entity);
      Pidf();
      Pidf(const Data& txt);
      Pidf(const HeaderFieldValue& hfv, const Mime& contentType);
      Pidf(const Data& txt, const Mime& contentType);
      Pidf(const Pidf& rhs);
      virtual ~Pidf();
      Pidf& operator=(const Pidf& rhs);

      /** @brief duplicate an Pidf object
          @return pointer to a new Pidf object  
        **/
      virtual Contents* clone() const;
      static const Mime& getStaticType() ;
      virtual EncodeStream& encodeParsed(EncodeStream& str) const;
      virtual void parse(ParseBuffer& pb);

      void setSimpleId(const Data& id);
      void setEntity(const Uri& entity);
      const Uri& getEntity() const;
      void setSimpleStatus(bool online, const Data& note = Data::Empty, 
                           const Data& contact = Data::Empty);
      bool getSimpleStatus(Data* note=NULL) const;
      
      Data& text() {checkParsed(); return mNote;}

      static bool init();   
   
      /** @deprecated
         @brief Deprecated
      */
      class Tuple
      {
         public:
            bool status;
            Data id;
            Data contact;
            QValue contactPriority;
            Data note;
            Data timeStamp;
            HashMap<Data, Data> attributes;
      };

      std::vector<Tuple>& getTuples();
      const std::vector<Tuple>& getTuples() const;
      int getNumTuples() const;

      // combine tuples
      void merge(const Pidf& other);
   
   private:
      Data mNote;
      Uri mEntity;
      std::vector<Tuple> mTuples;
};

EncodeStream& operator<<(EncodeStream& strm, const Pidf::Tuple& tuple);

static bool invokePidfInit = Pidf::init();

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
