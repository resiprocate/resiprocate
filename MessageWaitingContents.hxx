#ifndef MessageWaitingContents_hxx
#define MessageWaitingContents_hxx

#include <map>

#include "resiprocate/Contents.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Data.hxx"

namespace Vocal2
{

const char* skipSipLWS(ParseBuffer& pb);

typedef enum {mw_voice=0, mw_fax, mw_pager, mw_multimedia, mw_text, mw_none, MW_MAX} HeaderType;

class MessageWaitingContents : public Contents
{
   public:
      MessageWaitingContents();
      MessageWaitingContents(HeaderFieldValue* hfv, const Mime& contentType);
      MessageWaitingContents(const Data& data, const Mime& contentType);
      MessageWaitingContents(const MessageWaitingContents& rhs);
      ~MessageWaitingContents();
      MessageWaitingContents& operator=(const MessageWaitingContents& rhs);

      virtual Contents* clone() const;

      static const Mime& getStaticType() ;

      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual void parse(ParseBuffer& pb);

      class Header;

      Header& header(HeaderType ht) const;
      bool exists(HeaderType ht) const;
      void remove(HeaderType ht);

      class AccountHeader {};
      Uri& header(const AccountHeader& ht) const;
      bool exists(const AccountHeader& ht) const;
      void remove(const AccountHeader& ht);

      Data& header(const Data& hn) const;
      bool exists(const Data& hn) const;
      void remove(const Data& hn);

      bool& hasMessages() { return mHasMessages; }

      class Header
      {
         public:
            Header(unsigned int numNew,
                   unsigned int numOld);

            Header(unsigned int numNew,
                   unsigned int numOld,
                   unsigned int numUrgentNew,
                   unsigned int numUrgentOld);

            unsigned int& newCount() const {return mNew;}
            unsigned int& oldCount() const {return mOld;}
            bool& urgent() const {return mHasUrgent;}
            unsigned int& urgentNewCount() const {return mUrgentNew;}
            unsigned int& urgentOldCount() const {return mUrgentOld;}

         private:
            mutable unsigned int mNew;
            mutable unsigned int mOld;
            mutable bool mHasUrgent;
            mutable unsigned int mUrgentNew;
            mutable unsigned int mUrgentOld;

            friend class MessageWaitingContents;
      };

   private:
      void clear();

      bool mHasMessages;
      Uri* mAccountUri;
      Header* mHeaders[MW_MAX];
      
      mutable std::map<Data, Data> mExtensions;

      static ContentsFactory<MessageWaitingContents> Factory;
};

extern MessageWaitingContents::AccountHeader mw_account;

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
 *    notice, this std::list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this std::list of conditions and the following disclaimer in
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
