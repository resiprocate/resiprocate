#ifndef __P2P_SIGNATURE_CONTEXT_HXX
#define __P2P_SIGNATURE_CONTEXT_HXX 1

#include <map>
#include "rutil/Data.hxx"

#include "NodeId.hxx"
#include "Postable.hxx"
#include "ResourceId.hxx"
#include "UserName.hxx"
#include "Profile.hxx"

using resip::Data;
using std::vector; 

namespace p2p
{

class SignatureContext
{
   public:
      SignatureContext(Profile &config);
      
      // The types of hash function we support
      enum {
         sha1 = 1,
         sha256 = 2
      } HashAlgorithm;

      
      // Compute signatures, one function for each signature type
      // Returns an encoded "Signature" object
      Data computeSignatureWithPeerIdentity(const vector<const Data> toBeSigned);
      Data computeSignatureWithUserName(const vector<const Data> toBeSigned);
      Data computeSignatureWithCertificate(const vector<const Data> toBeSigned);


      // Fetch all the certificates corresponding to this set of
      // signatures, works in terms of encoded "Signature" objects
      void fetchCertificates(const vector<Data> signatures, Postable<bool> done);

      // Validate a signature, comparing to the expected NodeId node
      bool validateSignature(const vector<Data> toBeSigned, 
        const Data &signature, NodeId node);

      // Validate a signature, comparing to the expected UserName name
      bool validateSignature(const vector<Data> toBeSigned, 
        const Data &signature, UserName user);

      
   private:
     void digestData(const vector<Data> toBeSigned, unsigned char digest[32],
        unsigned int *digest_len);
      Data computeSignature(const vector<Data> toBeSigned);
      
      Profile &mProfile;

      class CachedCertificate 
      {
         public:
            CachedCertificate(X509 *x, bool isValid, UInt64 expires)
            {
               mCertificate=x;
               mValid=isValid;
               mExpiryTime=expires;
            }
            
            ~CachedCertificate()
            {
               X509_free(mCertificate);
            }
            
            
            X509        *mCertificate;    // The certificate
            bool        mValid;           // Is this entry valid
            UInt64      mExpiryTime;      // How long is this entry
                                          // trustable for
      };
         
      std::map<Data,CachedCertificate> mCertificateCache;

      
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
