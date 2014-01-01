#ifndef USER_INFO_MESSAGE_HXX
#define USER_INFO_MESSAGE_HXX 1

#include "resip/dum/UserAuthInfo.hxx"
#include "repro/ProcessorMessage.hxx"
#include "repro/AbstractDb.hxx"

namespace repro
{

class UserInfoMessage : public ProcessorMessage
{
   public:
      UserInfoMessage(Processor& proc,
                     const resip::Data& tid,
                     resip::TransactionUser* passedtu):
         ProcessorMessage(proc,tid,passedtu),
         mMode(resip::UserAuthInfo::Error)
      {
      }
      
      UserInfoMessage(const UserInfoMessage& orig):
         ProcessorMessage(orig)
      {
         mRec=orig.mRec;
      }

      const resip::Data& user() const{return mRec.user;}
      resip::Data& user(){return mRec.user;}
      
      const resip::Data& realm() const{return mRec.realm;}
      resip::Data& realm(){return mRec.realm;}
      
      const resip::Data& domain() const{return mRec.domain;}
      resip::Data& domain(){return mRec.domain;}
      
      const resip::Data& A1() const{return mRec.passwordHash;}
      resip::Data& A1(){return mRec.passwordHash;}

      resip::UserAuthInfo::InfoMode getMode() const { return mMode; }
      void setMode(const resip::UserAuthInfo::InfoMode m) { mMode = m; }
      
      virtual UserInfoMessage* clone() const {return new UserInfoMessage(*this);};

      virtual EncodeStream& encode(EncodeStream& ostr) const { ostr << "UserInfoMessage(tid="<<mTid<<")"; return ostr; };
      virtual EncodeStream& encodeBrief(EncodeStream& ostr) const { return encode(ostr);}
      
      AbstractDb::UserRecord mRec;
      resip::UserAuthInfo::InfoMode mMode;
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
