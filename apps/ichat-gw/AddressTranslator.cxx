
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"

#include "AppSubsystem.hxx"
#include "AddressTranslator.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::GATEWAY

using namespace gateway;
using namespace resip;
using namespace std;

namespace gateway 
{

AddressTranslator::AddressTranslator()
{  
}

AddressTranslator::~AddressTranslator()
{
    // Destroy all of the filters
    for(FilterOpList::iterator it = mFilterOperators.begin(); it != mFilterOperators.end(); it++)
    {
        if ( it->preq )
        {
            delete it->preq;
            it->preq = 0;
        }
    }
    mFilterOperators.clear();
}
      
void 
AddressTranslator::addTranslation(const resip::Data& matchingPattern,
                                  const resip::Data& rewriteExpression)
{ 
   InfoLog( << "Add translation " << matchingPattern << " -> " << rewriteExpression);
   
   FilterOp filter;
   filter.mMatchingPattern = matchingPattern;
   filter.mRewriteExpression =  rewriteExpression;

   if( !filter.mMatchingPattern.empty() )
   {
      // Note: this code originally used the PCRE (Perl Compatible) regular expression library. The ECMAScript standard is a subset
      // of the Perl regular expression syntax.  Posix regular expression syntax is quite a bit different (ie: std::regex_contacts::basic or 
      // std::regex_contacts::extended).  To be as backwards compatible with existing regular expressions as possible, we want to 
      // use the EMCAScript syntax.
      const std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
      if( filter.mRewriteExpression.find("$") == Data::npos )
      {
         flags |= std::regex_constants::nosubs;
      }
      try
      {
         filter.preq = new std::regex(filter.mMatchingPattern.c_str(), flags);
      }
      catch (std::regex_error& ex)
      {
         delete filter.preq;
         ErrLog( << "Translation has invalid match expression: "
                 << filter.mMatchingPattern );
         filter.preq = 0;
      }
   }
   mFilterOperators.push_back( filter ); 
}
     
void 
AddressTranslator::removeTranslation(const resip::Data& matchingPattern )
{  
   FilterOpList::iterator it = mFilterOperators.begin();
   while ( it != mFilterOperators.end() )
   {
      if ( it->mMatchingPattern == matchingPattern )
      {
         FilterOpList::iterator i = it;
         it++;
         if ( i->preq )
         {
           delete i->preq;
           i->preq = 0;
         }
         mFilterOperators.erase(i);
      }
      else
      {
         it++;
      }
   }
}

bool
AddressTranslator::translate(const resip::Data& address, resip::Data& translation, bool failIfNoRule)
{
   bool rc=false;
   DebugLog( << "Translating "<< address);

   for (FilterOpList::iterator it = mFilterOperators.begin();
        it != mFilterOperators.end(); it++)
   {
      Data rewrite = it->mRewriteExpression;
      Data& match = it->mMatchingPattern;
      if ( it->preq ) 
      {
         std::cmatch matches;
         
         // Note:  Using regex_search instead of regex_match, so that we don't need to fully match 
         //        the string, this is backwards compatible with the previous regexec PCRE implementation
         if(!std::regex_search(address.c_str(), matches, *it->preq))
         {
            // did not match 
            DebugLog( << "  Skipped translation "<< address << " did not match " << match );
            continue;
         }

         DebugLog( << "  Matched translation "<< address << " matched " << match );
         translation = rewrite;
         rc=true;
         if ( rewrite.find("$") != Data::npos )
         {
            for ( int i=1; i<matches.size(); i++)
            {
               Data subExp(matches[i]);
               DebugLog( << "  subExpression[" <<i <<"]="<< subExp );

               Data result;
               {
                  DataStream s(result);

                  ParseBuffer pb(translation);
                  
                  while (true)
                  {
                     const char* a = pb.position();
                     pb.skipToChars( Data("$") + char('0'+i) );
                     if ( pb.eof() )
                     {
                        s << pb.data(a);
                        break;
                     }
                     else
                     {
                        s << pb.data(a);
                        pb.skipN(2);
                        s <<  subExp;
                     }
                  }
                  s.flush();
               }
               translation = result;
            }
         }
         else
         {
            translation = rewrite;
            rc = true;
         }

         break;
      }
   }

   // If we didn't find a translation and we are not suppossed to fail, then just return address
   if(!rc && !failIfNoRule)
   {
      rc = true;
      translation = address;
   }

   InfoLog( << "Translated "<< address << " to " << translation);

   return rc;
}

}

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

