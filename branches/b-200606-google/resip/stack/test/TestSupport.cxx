#include "resip/stack/test/TestSupport.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

#include <iostream>
#include <iomanip>


using namespace std;


const int boxWidth = 4;
namespace resip 
{
class TestSupportPriv
{
private:
  static int chPerRow;
  friend class TestSupport;

public:
  static void fb(int w, char c=' ')
  {
    for(int i = 0 ; i < boxWidth-w ; i++) cout << c;
  };
  static void labels(int len, int row)
  {
    int start = chPerRow*row;
    cout << ' ';
    for(int i = 0; i < chPerRow && start+i < len; i++)
      {
        cout << setw(boxWidth-1) << start+i;
        fb(3);
      }
    cout << endl;
  }
  static void banner(int len, int row)
  {
    int chThisRow = 0;
    
    if (row >= len/chPerRow)
      chThisRow = len%chPerRow;
    else
      chThisRow = chPerRow;
    
    if (chThisRow < 1) return;
    
    cout << "+";
    for(int i = 0 ; i < chThisRow; i++)
      {
        fb(1,'-');
        cout << '+';
      }
    cout << endl;
    return;
  };
  static void data(const char * p , int len, int row)
  {
    
    cout << '|';
    for(int c = 0; c < chPerRow; c++)
      {
        int o = row*chPerRow + c;
        if (o >= len) break;
        char ch = p[o];
        
        if (isalnum(ch) || ispunct(ch) || ch == ' ' )
          {
            cout << ' ' << (char)ch;
               fb(3);
          }
        else if ( ch == '\t' )
          {
            cout << " \\t";
            fb(4);
          }
        else if ( ch >= '\t' || ch <= '\r')
          {
            cout << " \\" << "tnvfr"[ch-'\t'];
               fb(4);
          }
        else
          {
            cout << 'x' << hex << setw(2) << ch << dec;
            fb(4);
          }
        cout << '|';
      }
    cout << endl;
  };
  
};
  
  
void 
TestSupport::prettyPrint(const char * p,size_t len)
{
  
  size_t row = 0;
  if (TestSupportPriv::chPerRow == 0)
    {
      char * p = getenv("COLUMNS");
            if (p)
            {
               TestSupportPriv::chPerRow=strtol(p,0,0)/boxWidth;
            }
            else
            {
               TestSupportPriv::chPerRow = 80/boxWidth;
            }
         }
         
         for ( row = 0 ; row <= len/TestSupportPriv::chPerRow ; row++)
         {
            // do this row's banner
            TestSupportPriv::banner(len,row);
            // do this row's data
            TestSupportPriv::data(p,len,row);
            // do this row's banner
            TestSupportPriv::banner(len,row);
            // do this row's counts
            TestSupportPriv::labels(len,row);
         }
      };

Data
TestSupport::showN(const char * p, size_t l)
{
  Data s;
  for(unsigned int i = 0 ; i < l ; i++)
    {
      s += p[i];
    }
  return s;
}


int TestSupportPriv::chPerRow = 0;

SipMessage*
TestSupport::makeMessage(const Data& data, bool isExternal )
{
   return SipMessage::make(data, isExternal);
}

};


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
