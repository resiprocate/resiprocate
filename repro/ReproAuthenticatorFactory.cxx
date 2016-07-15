
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/Logger.hxx"

#include "AclStore.hxx"
#include "ReproAuthenticatorFactory.hxx"
#include "ReproRADIUSServerAuthManager.hxx"
#include "ReproServerAuthManager.hxx"
#include "ReproTlsPeerAuthManager.hxx"
#include "UserAuthGrabber.hxx"
#include "Worker.hxx"
#include "monkeys/CertificateAuthenticator.hxx"
#include "monkeys/DigestAuthenticator.hxx"
#include "monkeys/RADIUSAuthenticator.hxx"

using namespace std;
using namespace resip;
using namespace repro;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

ReproAuthenticatorFactory::ReproAuthenticatorFactory(ProxyConfig& proxyConfig, SipStack& sipStack, DialogUsageManager* dum)
    : mProxyConfig(proxyConfig),
      mSipStack(sipStack),
      mDum(dum),
      mEnableCertAuth(mProxyConfig.getConfigBool("EnableCertificateAuthenticator", false)),
      mEnableDigestAuth(!mProxyConfig.getConfigBool("DisableAuth", false)),
      mEnableRADIUS(mProxyConfig.getConfigBool("EnableRADIUS", false)),
      mRADIUSConfiguration(mProxyConfig.getConfigData("RADIUSConfiguration", "")),
      mStaticRealm(mProxyConfig.getConfigData("StaticRealm", "")),
      mDigestChallengeThirdParties(!mEnableCertAuth),
      mAuthRequestDispatcher(0),
      mCertificateAuthManager((DumFeature*)0),
      mServerAuthManager((ServerAuthManager*)0)
{
}

ReproAuthenticatorFactory::~ReproAuthenticatorFactory()
{
}

void
ReproAuthenticatorFactory::init()
{
   if(!mAuthRequestDispatcher.get())
   {
      int numAuthGrabberWorkerThreads = mProxyConfig.getConfigInt("NumAuthGrabberWorkerThreads", 2);
      if(numAuthGrabberWorkerThreads < 1)
      {
         numAuthGrabberWorkerThreads = 1; // must have at least one thread
      }
      std::auto_ptr<Worker> grabber(new UserAuthGrabber(mProxyConfig.getDataStore()->mUserStore));
      mAuthRequestDispatcher.reset(new Dispatcher(grabber, &mSipStack, numAuthGrabberWorkerThreads));
   }

   // TODO: should be implemented using AbstractDb
   loadCommonNameMappings();
}

void
ReproAuthenticatorFactory::loadCommonNameMappings()
{
   // Already loaded?
   if(!mCommonNameMappings.empty())
      return;

   Data mappingsFileName = mProxyConfig.getConfigData("CommonNameMappings", "");
   if(mappingsFileName.empty())
      return;

   InfoLog(<< "trying to load common name mappings from file: " << mappingsFileName);

   ifstream mappingsFile(mappingsFileName.c_str());
   if(!mappingsFile)
   {
      ErrLog(<< "failed to open mappings file: " << mappingsFileName << ", aborting");
      throw std::runtime_error("Error opening/reading mappings file");
   }

   string sline;
   while(getline(mappingsFile, sline))
   {
      Data line(sline);
      Data cn;
      PermittedFromAddresses permitted;
      ParseBuffer pb(line);

      pb.skipWhitespace();
      const char * anchor = pb.position();
      if(pb.eof() || *anchor == '#') continue;  // if line is a comment or blank then skip it

      // Look for end of name
      pb.skipToOneOf("\t");
      pb.data(cn, anchor);
      if(mCommonNameMappings.find(cn) != mCommonNameMappings.end())
      {
         ErrLog(<< "CN '" << cn << "' repeated in mappings file");
         throw std::runtime_error("CN repeated in mappings file");
      }
      pb.skipChar('\t');

      while(!pb.eof())
      {
         pb.skipWhitespace();
         if(pb.eof())
            continue;

         Data value;
         anchor = pb.position();
         pb.skipToOneOf(",\r\n ");
         pb.data(value, anchor);
         if(!value.empty())
         {
            StackLog(<< "Loading CN '" << cn << "', found mapping '" << value << "'");
            permitted.insert(value);
         }
         if(!pb.eof())
            pb.skipChar();
      }

      DebugLog(<< "Loaded mapping for CN '" << cn << "', " << permitted.size() << " mapping(s)");
      mCommonNameMappings[cn] = permitted;
   }
}

SharedPtr<DumFeature>
ReproAuthenticatorFactory::getCertificateAuthManager()
{
   init();
   if(!mCertificateAuthManager.get())
   {
      Store *db = mProxyConfig.getDataStore();
      resip_assert(db);
      AclStore& aclStore = db->mAclStore;
      mCertificateAuthManager.reset(new ReproTlsPeerAuthManager(*mDum, mDum->dumIncomingTarget(), aclStore, true, mCommonNameMappings));
   }
   return mCertificateAuthManager;
}

std::auto_ptr<Processor>
ReproAuthenticatorFactory::getCertificateAuthenticator()
{
   init();
   Store *db = mProxyConfig.getDataStore();
   resip_assert(db);
   AclStore& aclStore = db->mAclStore;
   return std::auto_ptr<Processor>(new CertificateAuthenticator(mProxyConfig, &mSipStack, aclStore, true, mCommonNameMappings));
}

SharedPtr<ServerAuthManager>
ReproAuthenticatorFactory::getServerAuthManager()
{
   init();
   if(!mServerAuthManager.get())
   {
      if(mEnableRADIUS)
      {
#ifdef USE_RADIUS_CLIENT
         mServerAuthManager.reset(new ReproRADIUSServerAuthManager(*mDum,
                                  mProxyConfig.getDataStore()->mAclStore,
                                  !mProxyConfig.getConfigBool("DisableAuthInt", false) /*useAuthInt*/,
                                  mProxyConfig.getConfigBool("RejectBadNonces", false),
                                  mRADIUSConfiguration,
                                  mDigestChallengeThirdParties,
                                  mStaticRealm));
#else
         ErrLog(<<"can't create ReproRADIUSServerAuthManager, not compiled with RADIUS support");
#endif
      }
      else
      {
         mServerAuthManager.reset(new ReproServerAuthManager(*mDum,
                                  getDispatcher(),
                                  mProxyConfig.getDataStore()->mAclStore,
                                  !mProxyConfig.getConfigBool("DisableAuthInt", false) /*useAuthInt*/,
                                  mProxyConfig.getConfigBool("RejectBadNonces", false),
                                  mDigestChallengeThirdParties,
                                  mStaticRealm));
      }
   }
   return mServerAuthManager;
}

std::auto_ptr<Processor>
ReproAuthenticatorFactory::getDigestAuthenticator()
{
   init();
   if(mEnableRADIUS)
   {
#ifdef USE_RADIUS_CLIENT
      return std::auto_ptr<Processor>(new RADIUSAuthenticator(mProxyConfig, mRADIUSConfiguration, mStaticRealm));
#else
      ErrLog(<<"can't create RADIUSAuthenticator, not compiled with RADIUS support");
      return std::auto_ptr<Processor>(0);
#endif
   }
   else
   {
      return std::auto_ptr<Processor>(new DigestAuthenticator(mProxyConfig, getDispatcher(), mStaticRealm));
   }
}

Dispatcher*
ReproAuthenticatorFactory::getDispatcher()
{
   init();
   return mAuthRequestDispatcher.get();
}

/* ====================================================================
 * BSD License
 *
 * Copyright (c) 2013 Daniel Pocock http://danielpocock.com All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
