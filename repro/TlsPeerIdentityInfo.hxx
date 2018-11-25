#ifndef TLSPEERIDENTITYINFO_MESSAGE_HXX
#define TLSPEERIDENTITYINFO_MESSAGE_HXX 1

#include "repro/ProcessorMessage.hxx"
#include "repro/AbstractDb.hxx"

namespace repro
{

class TlsPeerIdentityInfo : public ProcessorMessage
{
   public:
      TlsPeerIdentityInfo(Processor& proc,
                     const resip::Data& tid,
                     resip::TransactionUser* passedtu):
         ProcessorMessage(proc,tid,passedtu),
         mAuthorized(false)
      {
      }
      
      TlsPeerIdentityInfo(const TlsPeerIdentityInfo& orig):
         ProcessorMessage(orig)
      {
         mPeerNames = orig.mPeerNames;
         mIdentities = orig.mIdentities;
         mAuthorized = orig.mAuthorized;
      }

      const std::set<resip::Data>& peerNames() const{return mPeerNames;}
      std::set<resip::Data>& peerNames(){return mPeerNames;}
      
      const std::set<resip::Data>& identities() const{return mIdentities;}
      std::set<resip::Data>& identities(){return mIdentities;}

      const bool authorized() const{return mAuthorized;}
      bool& authorized(){return mAuthorized;}
      
      virtual TlsPeerIdentityInfo* clone() const {return new TlsPeerIdentityInfo(*this);};

      virtual EncodeStream& encode(EncodeStream& ostr) const { ostr << "TlsPeerIdentityInfo(tid="<<mTid<<")"; return ostr; };
      virtual EncodeStream& encodeBrief(EncodeStream& ostr) const { return encode(ostr);}
      
      std::set<resip::Data> mPeerNames;
      std::set<resip::Data> mIdentities;
      bool mAuthorized;
};

}
#endif

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 *
 */
