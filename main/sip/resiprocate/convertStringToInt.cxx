#include <iostream>

using namespace std;

inline int f(char *in)
{
   
   char str[4];
   register char * p = in;

   str[0] = *p++;
   str[1] = *p++;
   str[2] = *p++;
   str[3] = 0;

   return *(unsigned int*)str;
}


int
main()
{
#if 0   
   cout << "INV " << " " << hex << ((*(unsigned int*)"INV ") >> 8) << endl;
   cout << "ACK " << " " << ((*(unsigned int*)"ACK ") >> 8) << endl;
   cout << "BYE " << " " << ((*(unsigned int*)"BYE ") >> 8) << endl;
   cout << "CAN " << " " << ((*(unsigned int*)"CAN ") >> 8) << endl;
   cout << "REG " << " " << ((*(unsigned int*)"REG ") >> 8) << endl;
   cout << "SUB " << " " << ((*(unsigned int*)"SUB ") >> 8) << endl;
   cout << "OPT " << " " << ((*(unsigned int*)"OPT ") >> 8) << endl;
   cout << "REF " << " " << ((*(unsigned int*)"REF ") >> 8) << endl;
   cout << "NOT " << " " << ((*(unsigned int*)"NOT ")) << endl;
   cout << "NOT " << " " << ((*(unsigned int*)"NOT ") >> 8) << endl;
   cout << "NOT " << " " << ((*(unsigned int*)"NOTj") >> 8) << endl;
#else   
   cout << "INV " << " " << f("INV ") << endl;
   cout << "ACK " << " " << f("ACK ") << endl;
   cout << "BYE " << " " << f("BYE ") << endl;
   cout << "CAN " << " " << f("CAN ") << endl;
   cout << "REG " << " " << f("REG ") << endl;
   cout << "SUB " << " " << f("SUB ") << endl;
   cout << "OPT " << " " << f("OPT ") << endl;
   cout << "REF " << " " << f("REF ") << endl;
   cout << "NOT " << " " << f("NOT ") << endl;
   cout << "NOT " << " " << f("NOT ") << endl;
   cout << "NOT " << " " << f("NOT ") << endl;
#endif

   return 0;
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
