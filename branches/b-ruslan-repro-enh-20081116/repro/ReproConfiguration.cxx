#include "repro/ReproConfiguration.hxx"

namespace resip
{


   ReproConfiguration::ReproConfiguration():mLogType("cout"),
         mLogLevel("NONE"), mUdpPort(5060), mTcpPort(5060),
#if defined(USE_SSL)
         mTlsPort(5061),
#else
         mTlsPort(0),
#endif
         mRequestProcessorChainName("default"), mHttpPort(5080),mForkBehavior("EQUAL_Q_PARALLEL"),
         mCancelBetweenForkGroups(true), mWaitForTerminate(true), mMsBetweenForkGroups(3000), 
         mMsBeforeCancel(3000), mTimerC(180), mAdminPassword("admin"),
         mShouldRecordRoute(false), mDtlsPort(0), mUseV4(true), mUseV6(true), mNoChallenge(false), 
         mNoAuthIntChallenge(false), mNoWebChallenge(false), mNoRegistrar(false), mNoIdentityHeaders(false), 
         mCertServer(false), mRecursiveRedirect(0), mDoQValue(0), mAllowBadReg(0),mParallelForkStaticRoutes(0),
         mNoUseParameters(false), mNoLoadWebAdmin(false)
#ifdef WIN32
         , mInstallService(false), mRemoveService( false )
#endif

{
}

resip::Uri 
ReproConfiguration::toUri(const char* input, const char* description)
{
   resip::Uri uri;
   try
   {
      if (input)
      {
         uri = Uri(input);
      }
      else
      {
         std::cerr << "No " << description << " specified" << std::endl;
      }
   } 
   catch (ParseException& e)
   {
      std::cerr << "Caught: " << e << std::endl;
      std::cerr << "Can't parse " << description << " : " << input << std::endl;
      exit(-1);
   }
   return uri;
}

std::vector<resip::Data> 
ReproConfiguration::toVector(const char* input, const char* description)
{
   std::vector<Data> domains; 

   if (input)
   {
      Data buffer = input;
      if (input)
      {
         for (char* token = strtok(const_cast<char*>(buffer.c_str()), ","); token != 0; token = strtok(0, ","))
         {
            try
            {
               domains.push_back(token);
            } 
            catch (ParseException& e)
            {
               std::cout << "Caught: " << e << std::endl;
               std::cerr << "Can't parse " << description << " : " << token << std::endl;
               exit(-1);
            }
            catch (...)
            {
               std::cout << "Caught some exception" <<std::endl;
               std::cerr << "Some problem parsing " << description << " : " << token << std::endl;
            }
         }
      }
   }
   return domains;
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

/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */
