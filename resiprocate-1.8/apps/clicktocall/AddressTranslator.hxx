#if !defined(AddressTranslator_hxx)
#define AddressTranslator_hxx

#include <list>

#ifdef WIN32
#include <pcreposix.h>
#else
#include <regex.h>
#endif
#include "rutil/Data.hxx"

namespace clicktocall
{

class AddressTranslator
{
   public:
      AddressTranslator();
      ~AddressTranslator();
      
      void addTranslation(const resip::Data& matchingPattern,
                          const resip::Data& rewriteExpression);
      
      void removeTranslation(const resip::Data& matchingPattern);
                  
      bool translate(const resip::Data& address, resip::Data& translation, bool failIfNoRule=false);

   private:
      class FilterOp
      {
         public:
            resip::Data mMatchingPattern;
            resip::Data mRewriteExpression;
            regex_t *preq;
      };
      
      typedef std::list<FilterOp> FilterOpList;
      FilterOpList mFilterOperators; 
};

}
#endif  


/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

