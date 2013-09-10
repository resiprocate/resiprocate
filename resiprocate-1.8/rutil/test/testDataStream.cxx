#include "rutil/DataStream.hxx"

#include "rutil/Data.hxx"
#include "assert.h"
#include <iostream>

using namespace resip;
using namespace std;

int main()
{
   {
      // test overflow and synch with flush
      char buf[16];
      Data data(Data::Borrow, buf, sizeof(buf));
      data.clear();

      DataStream str(data);
      for (int i = 0; i < 32; ++i) {
         str << 'a';
         str.flush();
      }
   }

   {
      Data foo;
      {
         oDataStream str(foo);

         str << "some characters " << 42 << " for foo";
         str << "some more characters " << 24 << " for foo";

         str.flush();

         str << "some a few more characters ";

         str.reset();

         str << "all that remains";
      }

      std::cerr << "!! " << foo << std::endl;

      assert(foo == "all that remains");
   }

   {
      Data d;
      d = ("  SIP invitations used to create sessions carry session descriptions\n"
         "that allow participants to agree on a set of compatible media types.\n"
         "SIP makes use of elements called proxy servers to help route requests\n"
         "to the user's current location, authenticate and authorize users for\n"
         "services, implement provider call-routing policies, and provide\n"
         "features to users.  SIP also provides a registration function that\n"
         "allows users to upload their current locations for use by proxy\n"
         "servers.  SIP runs on top of several different transport protocols."
         );

      d.clear();
      DataStream s(d);
      s << "all the kings horses and all the kings men...";
      s.flush();

      cerr << d.size() << endl;
      cerr << d.c_str();

      assert(strcmp(d.c_str(), "all the kings horses and all the kings men...") == 0);
   }

   {
      Data d;
      DataStream ds(d);

      Data foo("foo");

      ds << "Here is some stuff " << foo << 17 << ' ' << 'c' << ' ' << -157 << endl;
      cerr << "!! <" << d << ">" << endl;
      ds.flush();
      assert(d == "Here is some stuff foo17 c -157\n");
      assert(strlen(d.c_str()) == d.size());
   }
   {
      Data d(500, Data::Preallocate);
      DataStream ds(d);

      Data foo("foo");

      ds << "Here is some stuff " << foo << 17 << ' ' << 'c' << ' ' << -157 << endl;
      cerr << "!! <" << d << ">" << endl;
      ds.flush();
      assert(d == "Here is some stuff foo17 c -157\n");
      assert(strlen(d.c_str()) == d.size());
   }

   {
      Data d(3, Data::Preallocate);
      DataStream ds(d);

      for (int i = 0; i < 200; i++)
      {
         ds << Data(i) << ' ';
      }

      ds.flush();

      assert(d == "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 ");
   }

   cerr << "All OK" << endl;
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
