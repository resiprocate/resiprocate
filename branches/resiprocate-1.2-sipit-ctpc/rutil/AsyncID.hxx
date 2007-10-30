#if !defined(RESIP_ASYNC_ID_HXX)
#define RESIP_ASYNC_ID_HXX

namespace resip
{

// ?dcm? -- I originally wanted a flexible class that would map to any asyncronous
// completetion token.  One way to do this is with templates in the function
// that accept Async ids, another way is to have them passed around by
// reference, but in the CounterPath case(for example) ids are simle ulongs, so this
// would cause a lot of work in the glue code.  Of course, this definition is
// located here, so it would be easy to change to a lightweight wrapper class to
// other implementation.  A templatized class w/ a conversion operator would
// work.
// sometimes GC and handles make life easier.  Of course the template approach
// could involve ref. counting when necessary.

//should be small enough to happily pass by value
//   class AsyncID
//   {
//   public:
//     virtual operator==(const AsyncID& other) const = 0;
//     virtual bool operator<(const AsyncID& rhs) const = 0;
//   };

typedef unsigned long AsyncID;

//error code of zero is success. Should define an enum for implementations to
//map to.
class AsyncResult
{
   public:
      AsyncResult() : mErrorCode(0) {}
      AsyncResult(long errorCode) : mErrorCode(errorCode) {}
      void setError(long errorCode) { mErrorCode = errorCode; }
      long errorCode() const { return mErrorCode; }
      bool success() const { return mErrorCode == 0 ? true : false; }
   protected:
      long mErrorCode;
};

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
