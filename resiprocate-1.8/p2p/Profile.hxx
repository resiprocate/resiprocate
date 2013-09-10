#ifndef __P2P_PROFILE_HXX
#define __P2P_PROFILE_HXX 1

#include <openssl/evp.h>
#include <openssl/x509.h>

#ifndef WIN32
#include <arpa/inet.h> // For htonl
#endif

#include "rutil/Data.hxx"
#include "rutil/GenericIPAddress.hxx"

#include "p2p/NodeId.hxx"
#include "p2p/UserName.hxx"

namespace p2p
{

enum {RELOAD_APPLICATION_ID = 8675};

class Profile
{
public:
      Profile() : mNodeId(), mNumInitialFingers(8) {}
      virtual ~Profile() {;}
      
      virtual const X509        *getCertificate() { return 0; }
      virtual const EVP_PKEY    *getPrivateKey() { return 0; }
      
      virtual resip::Data& signatureDigest() { return mSignatureDigest; }
      virtual const resip::Data& signatureDigest() const { return mSignatureDigest; }   
      
      virtual resip::Data& overlayName() { return mOverlayName; }
      virtual const resip::Data& overlayName() const { return mOverlayName; }
      
      virtual bool& isBootstrap() { return mBootstrap; }
      virtual const bool isBootstrap() const { return mBootstrap; }
      
      virtual NodeId& nodeId() { return mNodeId; }
      virtual const NodeId nodeId() const { return mNodeId; }
      
      virtual UserName& userName() { return mUserName; }
      virtual const UserName& userName() const { return mUserName; }
      
      virtual unsigned int& numInitialFingers() { return mNumInitialFingers; }
      virtual const unsigned int numInitialFingers() const { return mNumInitialFingers; }
      
      virtual std::vector<resip::GenericIPAddress>& bootstrapNodes() { return mBootstrapNodes; }
      virtual const std::vector<resip::GenericIPAddress>& bootstrapNodes() const { return mBootstrapNodes; }
      
   private:
      resip::Data mOverlayName;
      resip::Data mSignatureDigest;
      NodeId      mNodeId;
      UserName    mUserName;
      unsigned int mNumInitialFingers;
      std::vector<resip::GenericIPAddress> mBootstrapNodes;
      bool mBootstrap;
};

}


#endif

/* ======================================================================
 *  Copyright (c) 2008, Various contributors to the Resiprocate project
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - The names of the project's contributors may not be used to
 *        endorse or promote products derived from this software without
 *        specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================== */
