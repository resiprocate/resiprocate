static const char* const Data_cxx_Version =
"$Id: Data.cxx,v 1.2 2002/09/22 01:16:35 fluffy Exp $";

#include <cstdio>
#include <ctype.h>
#include <cctype>
#include <algorithm>

#include <sipstack/Data.hxx>

#define INLINE_ inline

using namespace Vocal2;
using namespace std;


static INLINE_ 
bool isIn(char c, const char* match)
{
   char p;

   while((p = *match++))
   {
      if(p == c)
      {
         return true;
      }
   }

   return false;
}


Data::Data() : string()
{
}

Data::Data( const char* str, int length ) :
   string(str,length)
{
}

Data::Data( const char* str ) :
   string(str)
{
}

Data::Data( const string& str) :
   string(str)
{
}

Data::Data( const int value) 
{
   // !ah! sprintf needs a punt
   char buffer[32];
   
   sprintf(buffer,"%d",value);
   *this = buffer; // !ah! ie
}

Data::Data( const Data& data ) : string(data)
{
}

int
Data::convertInt() const
{
   size_t l = size();
   int val = 0;
   size_t p = 0;

   while (l--)
   {
      char c = (*this)[p++];
      if ((c >= '0') && (c <= '9'))
      {
         val *= 10;
         val += c - '0';
      }
      else
      {
         return val;
      }
   }
   return val;
}


int Data::find( const Data& match, int start)
{
   return static_cast < int > (find(match, static_cast < string::size_type > (start)));
}

int Data::match( const Data& match, Data* retModifiedData, bool replace, const Data& replaceWith)
{
   string::size_type pos = find(match);
   if (pos == string::npos)
   {
      return NOT_FOUND;
   }

   string::size_type replacePos = pos + match.size();
   int retVal = FIRST;

   if (retModifiedData)
   {
      (*retModifiedData) = substr(0, pos);
      if (retModifiedData->size()) retVal = FOUND;
   }

   if (replace)
   {
      if (replacePos <= size() )
      {
         this->replace(0, replacePos, replaceWith);
      }
      else
      {
         assert(0);
      }
   }

   return retVal;
}

bool isEqualNoCase(const Data& left, const Data& right )
{
   string::const_iterator leftIter = left.begin();
   string::const_iterator rightIter = right.begin();

   while ( (leftIter != left.end()) && (rightIter != right.end()) )
   {
      if (toupper(*leftIter) != toupper(*rightIter))
      {
         return false;
      }
      ++leftIter;
      ++rightIter;
   }

   if ( (leftIter != left.end()) || (rightIter != right.end()) )
   {
      // since both aren't the same length, they're not equal
      return false;
   }

   return true;
}

void Data::removeSpaces()
{
   //removes spaces before and after the characters.
   //Leaves the embedded spaces as is.

   if (size() == 0)
   {
      return ;
   }

   string::size_type beforeval;
   do
   {
      beforeval = find(SPACE);

      if (beforeval != 0)
      {
         break;
      }

      replace(beforeval, strlen(SPACE) , "");


   }
   while (beforeval == 0);


   string::size_type afterval;
   do
   {
      //afterSpace="";
      //proceed to the last character in Data.
      // string tempstring = buf;
      afterval = rfind(SPACE);

      //if there are chars after val, discard .
      if (afterval + 1 < size())
      {
         break;
      }

      replace(afterval, strlen(SPACE) , "");

      //Data tempdata(tempstring);

      //*data = tempdata;
   }

   while (afterval != string::npos);

}

void
Data::expand(const Data& startFrom, const Data& findstr, const Data& replstr, const Data& delimiter)
{
   string::size_type startPos = find(startFrom);
   if (startPos < string::npos)
   {
      string::size_type delimPos = find(delimiter, startPos);
      string::size_type findPos = find( findstr, startPos);

      while (findPos < delimPos)
      {
         //found replstr, replace
         replace( findPos, findstr.size(), replstr);
         delimPos = find( delimiter, findPos + replstr.size());
         findPos = find( findstr, findPos);
      }
   }
}

Data
Data::substring(int first, int last) const
{
   Data x;
   if (last == -1)
   {
      last = string::npos;
   }
   else
   {
      last = last - first;
   }
   return substr(first, last);
}

void
Data::lowercase()
{
   std::transform(begin(),end(),begin(),::tolower);
}

void 
Data::uppercase()
{
   std::transform(begin(),end(),begin(),::toupper);
}

Data 
Data::parseOutsideQuotes(const char* match, 
			       bool useQuote,
			       bool useAngle,
			       bool* matchFail /* default argument */ )
{
   size_t pos = 0;

   bool inDoubleQuotes = false;
   bool inAngleBrackets = false;

   bool foundAny = false;

   size_t bufsize = size();

   while(!foundAny && (pos < bufsize))
   {
      char p = (*this)[pos];

      switch (p)
      {
         case '"':
            if(!inAngleBrackets && useQuote)
            {
               inDoubleQuotes = !inDoubleQuotes;
            }
            break;
         case '<':
            if(!inDoubleQuotes && useAngle)
            {
               inAngleBrackets = true;
            }
            break;
         case '>':
            if(!inDoubleQuotes && useAngle)
            {
               inAngleBrackets = false;
            }
            break;
         default:
            break;
      }

      if(!inDoubleQuotes && !inAngleBrackets && isIn(p, match))
      {
         foundAny = true;
      }
      pos++;
   }

   size_t pos2 = pos;

   while(
      foundAny && 
      (pos2 < bufsize) && 
      isIn((*this)[pos2], match)
      )
   {
      pos2++;
   }

   Data result;

   if(foundAny)
   {
      result = *this;
      result = substr(0, pos - 1);

      *this = substr(pos2, size());
      if(matchFail)
      {
         *matchFail = false;
      }
   }
   else
   {
      if(matchFail)
      {
         *matchFail = true;
      }
   }

   return result;
}

Data 
Data::parse(const char* match, bool* matchFail /* default argument */ )
{
   size_t pos = 0;

   bool foundAny = false;

   size_t bufsize = size();

   while(!foundAny && (pos < bufsize))
   {
      char p = (*this)[pos];
      if(isIn(p, match))
      {
         foundAny = true;
      }
      pos++;
   }

   size_t pos2 = pos;

   while(
      foundAny && 
      (pos2 < bufsize) && 
      isIn((*this)[pos2], match)
      )
   {
      pos2++;
   }

   Data result;

   if(foundAny)
   {
      result = substr(0, pos - 1);

      (*this) = substr(pos2, size());
      if(matchFail)
      {
         *matchFail = false;
      }
   }
   else
   {
      if(matchFail)
      {
         *matchFail = true;
      }
   }

   return result;
}

Data 
Data::matchChar(const char* match, 
		      char* matchedChar /* default argument */)
{
   size_t pos = 0;

   bool foundAny = false;

   size_t bufsize = size();

   while(!foundAny && (pos < bufsize))
   {
      char p = (*this)[pos];
      if(isIn(p, match))
      {
         foundAny = true;
         if(matchedChar)
         {
            *matchedChar = p;
         }
      }
      pos++;
   }

   Data result;

   if(foundAny)
   {
      result = substr(0, pos - 1);

      (*this) = substr(pos, size());
   }
   else if(matchedChar)
   {
      *matchedChar = '\0';
   }

   return result;
}

Data 
Data::getLine(bool* matchFail /* default argument */ )
{
   const int STARTING = 0;
   const int HAS_CR = 1;
   const int HAS_LF = 2;
   const int HAS_CRLF = 3;

   int state = STARTING;
   size_t pos = 0;

   bool foundAny = false;

   size_t bufsize = size();

   while(!foundAny && (pos < bufsize))
   {
      char p = (*this)[pos];
      if( p == '\r' )
      {
         state = HAS_CR;
      }
      else if (p == '\n' )
      {
         if(state == HAS_CR)
         {
            state = HAS_CRLF;
         }
         else
         {
            state = HAS_LF;
         }
         foundAny = true;
      }
      else
      {
         state = STARTING;
      }
      pos++;
   }

   int pos2 = pos;

   if(state == HAS_CRLF)
   {
      pos--;
   }

   Data result;

   if(foundAny)
   {
      result = substr(0, pos - 1);

      (*this) = substr(pos2, size());
      if(matchFail)
      {
         *matchFail = false;
      }
   }
   else
   {
      if(matchFail)
      {
         *matchFail = true;
      }
   }

   return result;
}

void 
Data::removeLWS()
{
   size_t replaceTo;
   size_t pos;

   pos = find ("\r\n");
   if (pos == string::npos)
   {
      pos = find("\n");
   }
   else
   {
      replaceTo = pos + 1; //should end after \r\n
   }
   if (pos == string::npos)
   {
      return;
   }
   else
   {
      replaceTo = pos; //should end after \n
   }
   bool replaceFlag = false;
   do
   {
      while ( (replaceTo + 1 < size()) && 
              ( ((*this)[replaceTo+1] == '\t') ||
                ((*this)[replaceTo+1] == ' ')
                 ) 
         )
      {
         char temp = (*this)[replaceTo];
         replaceTo++;
         temp = (*this)[replaceTo];
         replaceFlag = true;
      }
      if (replaceFlag)
      {
         int replaceFrom = pos;
         while ( (replaceFrom-1 > 0) &&
                 ( ((*this)[replaceFrom-1] == '\t') ||
                   ((*this)[replaceFrom-1] == ' ')
                    )
            )
         {
            char temp = (*this)[replaceFrom];
            replaceFrom--;
            temp = (*this)[replaceFrom];
         }
         int replaceNum = replaceTo - replaceFrom;
         replace(replaceFrom, replaceNum, ""); //replace with nothing
	   
      }
      //remember pos.
      size_t initpos = pos;
      pos = find("\r\n", initpos+2);
      if (pos > size())
      {
         pos = find("\n", initpos+2);
         replaceTo = pos;
      }
      else
      {
         replaceTo = pos+1;
      }
   }
   while (pos < size());
}

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

