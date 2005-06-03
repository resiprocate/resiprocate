#include <iostream>
#include <string>
#include <assert.h>

// djb2 hash swiped from the net, credited to bernstein
unsigned int
hash(const char *str, unsigned int len)
{
   unsigned int hash = 5381;
   int c;
   int i = 0;

   while (c = *str++)
   {
      if (i == len)
      {
         break;
      }
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
   }
   hash = ((hash << 5) + hash) + len;
   return hash;
}

// unrolled and case insensitized
unsigned int 
hashcasen(const char* data, 
          unsigned int len)
{
   unsigned int x = 5381;
   switch (len)
   {
      case 1 :
         return (x << 5) + x + (0x40 | *data);
      case 2 : 
      {
         x = (x << 5) + x + (0x40 | *data);
         return (x << 5) + x + (0x40 | *(data+1));
      }
      case 3 : 
      {
         x = (x << 5) + x + (0x40 | *data);
         x = (x << 5) + x + (0x40 | *(data+1));
         return (x << 5) + x + (0x40 | *(data+2));
      }
      default :
      {
         x = (x << 5) + x + (0x40 | *data);
         x = (x << 5) + x + (0x40 | *(data+len-1));
         x = (x << 5) + x + (0x40 | *(data+2));
         x = (x << 5) + x + (0x40 | *(data+len-2));
         return (x << 5) + x + len;
      }
   }
}

// strategy:
// use sparse parallel arrays of size num_buckets + 1
// name => enum
// the name is stored by hashcasen(name) % num_buckets
// if the position is occupied, move up the array until a free spot is found
// on lookup:
//    if the name array [hash] is empty, return UNKNOWN
//    if the name array[hash] is occupied and equal, found, return enum[hash]
//    if the name array[hash] is occupied and !equal, try hash++

int 
main()
{
   std::string parameters[17] = 
      {
         "transport",
         "user",
         "method",
         "ttl",
         "maddr",
         "lr",
         "q",
         "purpose",
         "expires",
         "handling",
         "tag",
         "duration",
         "branch",
         "received",
         "comp",
         "rport"
      };

   std::string PARAMETERS[17] = 
      {
         "TRANSPORT",
         "USER",
         "METHOD",
         "TTL",
         "MADDR",
         "LR",
         "Q",
         "PURPOSE",
         "EXPIRES",
         "HANDLING",
         "TAG",
         "DURATION",
         "BRANCH",
         "RECEIVED",
         "COMP",
         "RPORT"
      };


   for (int i = 0; i < 16; i++)
   {
      std::cerr << parameters[i] << " => " << hashcasen(parameters[i].c_str(), parameters[i].size()) % 73 << std::endl;
      assert(hashcasen(parameters[i].c_str(), parameters[i].size()) == hashcasen(parameters[i].c_str(), PARAMETERS[i].size()));
   }
   std::cerr << std::endl;
   std::cerr << std::endl;

   std::string headers[] = 
      {
         "Body",
         "CSeq",
         "Call_ID",
         "Contact",
         "Content_Length",
         "Expires",
         "From",
         "Max_Forwards",
         "Route",
         "Subject",
         "To",
         "Via",
         "Accept",
         "Accept_Encoding",
         "Accept_Language",
         "Alert_Info",
         "Allow",
         "Authentication_Info",
         "Authorization",
         "Call_Info",
         "Content_Disposition",
         "Content_Encoding",
         "Content_Language",
         "Content_Type",
         "Date",
         "Error_Info",
         "In_Reply_To",
         "Min_Expires",
         "MIME_Version",
         "Organization",
         "Priority",
         "Proxy_Authenticate",
         "Proxy_Authorization",
         "Proxy_Require",
         "Record_Route",
         "Reply_To",
         "Require",
         "Retry_After",
         "Server",
         "Supported",
         "Timestamp",
         "Unsupported",
         "User_Agent",
         "Warning",
         "WWW_Authenticate",
         "Subscription_State"
      };

   for (int i = 0; i < 46; i++)
   {
      std::cerr << headers[i] << " => " << hashcasen(headers[i].c_str(), headers[i].size()) % 505 << std::endl;
   }
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
