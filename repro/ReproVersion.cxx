
/*
** Build system provides these.
** VERSION comes from a file and is the 'marketing' version
**          of repro (major.minor)
** BUILD_REV is the subversion svnversion output (lightly formatted)
**           (ends in M if there are local modifications)
** RELEASE_VERSION
*/

#include <string>

#if defined(TESTDRIVER)
#include <iostream>
#endif

#include "repro/ReproVersion.hxx"
#include "repro/reproInfo.hxx"
#if !defined(REPRO_BUILD_REV)
# define REPRO_BUILD_REV "000000"
#endif

#if !defined(REPRO_BUILD_HOST)
# define REPRO_BUILD_HOST "unknown.invalid"
#endif

#if !defined(REPRO_RELEASE_VERSION)
# define REPRO_RELEASE_VERSION "0.0"
#endif

#if !defined(REPRO_NAME)
# define REPRO_NAME "Repro"
#endif



namespace repro
{
   VersionUtils::VersionUtils():
      mBuildHost(REPRO_BUILD_HOST),
      mReleaseVersion(REPRO_RELEASE_VERSION),
      mScmRevision(REPRO_BUILD_REV),
      mDisplayVersion(REPRO_NAME),
      mBuildStamp(REPRO_BUILD_REV)
   {
      mDisplayVersion += ' ';
      mDisplayVersion += mReleaseVersion;
      mDisplayVersion += '/';

      mBuildStamp += '@';
      mBuildStamp += mBuildHost;

      mDisplayVersion += mBuildStamp;
   }

   VersionUtils* VersionUtils::sVU = 0;

   const VersionUtils&
   VersionUtils::instance()
   {
      if (sVU == 0)
      {
         sVU = new VersionUtils;
      }
      return *sVU;
   }

   VersionUtils::~VersionUtils() {};

   const std::string&
   VersionUtils::buildStamp() const
   {
      return  mBuildStamp;
   }
   
   const std::string&
   VersionUtils::releaseVersion() const
   {
      return mReleaseVersion;
   }


   const std::string&
   VersionUtils::buildHost() const
   {
      return mBuildHost;
   }

   const std::string&
   VersionUtils::displayVersion() const
   {
      return mDisplayVersion;
   }

   const std::string&
   VersionUtils::scmRevision() const
   {
      return mScmRevision;
   }

};


#if defined(TESTDRIVER)
int main()
{
#define T(x) std::cout << #x << " = " << repro::VersionUtils::instance().x() << std::endl;
   T(displayVersion);
   T(buildStamp);
   T(scmRevision);
   T(releaseVersion);
   T(buildHost);
   return 0;
}
#undef T
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
 */
