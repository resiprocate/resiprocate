#if !defined(DUM_CommandLineParser_hxx)
#define DUM_CommandLineParser_hxx

#include <vector>
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"

class CommandLineParser
{
   public:
      CommandLineParser(int argc, char** argv);
      static resip::Uri toUri(const char* input, const char* description);
      static std::vector<resip::Uri> toUriVector(const char* input, const char* description);
      static std::vector<resip::Data> toDataVector(const char* input, const char* description);
      static std::set<int> toIntSet(const char* input, const char* description);

      resip::Data mLogType;
      resip::Data mLogLevel;
      bool mInteractive;
      resip::Data mTlsDomain;
      resip::Data mProxyHostName;
      resip::Data mUserIPAddr;
      std::vector<resip::Data> mMultihomedAddrs;
      resip::Data mEnumSuffix;
      resip::Uri mRecordRoute;
      std::set<int> mUdpPorts;
      std::set<int> mTcpPorts;
      std::set<int> mTlsPorts;
      std::set<int> mDtlsPorts;
      bool mNoV4;
      bool mNoV6;
      bool mThreadedStack;
      bool mUseCongestionManager;

      resip::Data mCertPath;
      bool mEnableFlowTokenHack;
      bool mNoChallenge;
      bool mNoWebChallenge;
      bool mNoRegistrar;
      bool mCertServer;
      resip::Data mRequestProcessorChainName;
      resip::Data mMySqlServer;
      int mHttpPort;

      bool mEncrypt;
      bool mSign;
      bool mGenUserCert;
      
      int mRegisterDuration;
      
      resip::Uri mAor;
      resip::Data mPassword;
      
      resip::Uri mOutboundProxy;
      resip::Uri mContact;
      std::vector<resip::Uri> mBuddies;
      resip::Uri mTarget;
      resip::Data mPassPhrase;
};
 
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
