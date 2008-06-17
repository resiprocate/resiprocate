#ifndef P2P_DictionayValue_hxx
#define P2P_DictionaryValue_hxx

#include "rutil/Data.hxx"
#include <map>

namespace p2p
{

class DictionaryValue public: AbstractValue 
{
  public:
   DictionaryValue()
      : mMap()
   {}
      
   virtual std::vector<const resip::Data> collectSignableData() const
   {
      assert(0);
   }

//    const resip::Data& get(const resip::Data& key) const;
//    void set(const resip::Data& key, const resip::Data& value);
//    void clear(const resip::Data& key);
   std::map<const resip::Data, const Entry>& dictionary();

  private:
   std::map<const resip::Data, const Entry> mMap;

   class Entry public: Signable {
      Entry(const resip::Data& key, const resip::Data& value)
         : mKey(key),
         mValue(value)
         {}
      
      virtual std::vector<const resip::Data> collectSignableData() const
      {
         std::vector<const resip::Data> result;
         result.add(resip::Data(resip::Data::Borrow, mKey));
         result.add(resip::Data(resip::Data::Borrow, mValue));
         return result;
      }

      bool exists() 
      {
         return mValue == SingleValue::NoValue;
      }
      
     private:
      const resip::Data mKey;
      const resip::Data mValue;
  };
};

} // p2p

#endif // P2P_DictionaryValue_hxx


/* ======================================================================
 *  Copyright (c) 2008, Various contributors to the Resiprocate project
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - The names of the project's contributors may not be used to
 *        endorse or promote products derived from this software without
 *        specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================== */
